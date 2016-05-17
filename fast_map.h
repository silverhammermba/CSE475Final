#ifndef FAST_MAP_H
#define FAST_MAP_H

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "fast_lookup_map.h"

template <class K, class V>
class FastMap
{
	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<const K, V> pair_t;
	typedef FastLookupMap<K, V> subtable_t;
	typedef std::vector<subtable_t*> table_t;
	typedef std::vector<pair_t*> st_table_t; // the internal table type for the subtables (used during rebuilds)

	typedef boost::upgrade_lock<boost::upgrade_mutex> read_lock_t;
	typedef boost::unique_lock<boost::upgrade_mutex> write_lock_t;
	typedef boost::upgrade_to_unique_lock<boost::upgrade_mutex> readwrite_lock_t;
public:
	// construct with a hint that we need to store at least num_pairs pairs
	FastMap(size_t num_pairs = 0)
		: m_num_operations{0},
		m_num_pairs{0},
		m_threshold{thresholdFromNumPairs(num_pairs)}
	{
		rebuildLocked();
	}

	~FastMap()
	{
		for (auto& st_bucket : m_table) delete st_bucket;
	}

	size_t size() const
	{
		read_lock_t rlock(m_mutex);
		return m_num_pairs;
	}

	// try to insert pair into the hash table
	bool insert(const pair_t& pair)
	{
		read_lock_t rlock(m_mutex);

		// check for duplicate key
		if (count(pair.first)) return false;

		readwrite_lock_t rwlock(rlock);

		++m_num_operations;
		++m_num_pairs;

		// after a certain number of successful inserts, do a rebuild regardless
		if (m_num_operations > m_threshold) return insertAndRebuildLocked(pair);

		auto& st_bucket = getSubtableLocked(pair.first);

		// create subtable if it doesn't exist
		if (!st_bucket) st_bucket = new FastLookupMap<K, V>();

		// if we can insert without growing the subtable, do that
		if (st_bucket->isUnderCapacity()) return st_bucket->insert(pair);

		// else we need to see what the effect of adding the pair to the subtable would be

		// Calculate new accumulated subtable allocation
		// sum of s_j
		/* TODO there may be a problem with excessive rebuilding.
		 * since the subtales' capacities never shrink, if we add many pairs
		 * then delete many pairs, we might always see the table as unbalanced
		 * afterwards, until we add enough pairs to bump the threshold back up
		 */
		size_t num_buckets = st_bucket->bucketCountAfterInsert();
		for (auto& stb : m_table)
		{
			if (stb && stb != st_bucket) num_buckets += stb->bucketCount();
		}

		// if the insert would be balanced, do that
		if (isBucketCountBalancedLocked(num_buckets)) return st_bucket->insert(pair);

		// else insert would unbalance table, rebuild
		return insertAndRebuildLocked(pair);
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		read_lock_t rlock(m_mutex);

		if (!count(key)) return 0;

		readwrite_lock_t rwlock(rlock);

		++m_num_operations;
		auto& st_bucket = getSubtableLocked(key);
		st_bucket->erase(key);
		--m_num_pairs;

		if (m_num_operations >= m_threshold) rebuildLocked();

		return 1;
	}

	// return the value matching key
	V at(const K& key) const
	{
		read_lock_t rlock(m_mutex);

		if (!count(key)) throw std::out_of_range("FastMap::at");
		auto& usubtable = getSubtableLocked(key);
		return !usubtable ? 0 : usubtable->at(key);
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		read_lock_t rlock(m_mutex);
		auto& st_bucket = getSubtableLocked(key);
		return st_bucket && st_bucket->count(key);
	}

	// rebuild the entire table
	void rebuild()
	{
		write_lock_t wlock(m_mutex);
		insertAndRebuildLocked(nullptr);
	}

private:
	// constants determining growth rates. TODO what happens when we vary these?
	static const size_t THRESHOLD_SCALE = 2; // c, controls how the threshold scales based on number known pairs
	static const size_t ST_BUCKET_SCALE = 3; // controls how number of buckets (for subtables) scales with the threshold
	/* XXX in the paper they choose this to be 8 * sqrt(30) / 15 = 2.921,
	 * in order to prove the linear space requirement of the map
	 */

	// what should the threshold be if we need to store num_pairs pairs?
	// (1 + c) * n
	static size_t thresholdFromNumPairs(size_t num_pairs)
	{
		return (1 + THRESHOLD_SCALE) * std::max(num_pairs, (size_t)4);
	}

	// how many subtable buckets should we have for the current threshold?
	// s(M)
	static size_t stBucketCountFromThreshold(size_t threshold)
	{
		return ST_BUCKET_SCALE * threshold;
	}

	// return 1 if pair matching key is in table, else return 0
	size_t countLocked(const K& key) const
	{
		auto& st_bucket = getSubtableLocked(key);
		return st_bucket && st_bucket->count(key);
	}

	// convenience functions for getting the subtable bucket for a key
	// XXX m_mutex MUST be owned (shared or unique) by the calling thread
	subtable_t*& getSubtableLocked(const K& key)
	{
		return m_table.at(m_hash(key));
	}

	subtable_t* const& getSubtableLocked(const K& key) const
	{
		return m_table.at(m_hash(key));
	}

	// rebuild the entire table
	void rebuildLocked()
	{
		insertAndRebuildLocked(nullptr);
	}

	// insert a new pair (if non-null) and rebuild the entire table
	bool insertAndRebuildLocked(const pair_t& new_pair)
	{
		return insertAndRebuildLocked(new pair_t(new_pair));
	}

	// insert a new pair (if non-null) and rebuild the entire table
	bool insertAndRebuildLocked(pair_t* new_bucket)
	{
		// if the table is empty (and we aren't inserting) rebuilding is easy
		if (!new_bucket && m_num_pairs == 0)
		{
			m_table.resize(stBucketCountFromThreshold(m_threshold));
			m_hash = random_hash<K>(m_table.size());
			m_num_operations = 0;
			return false;
		}

		// assume that non-null pair will be inserted
		bool inserted = new_bucket != nullptr;

		// ensure duplicate keys are not added
		if (new_bucket && countLocked(new_bucket->first))
		{
			delete new_bucket;
			new_bucket = nullptr;
			inserted = false;
		}

		// move all pairs from subtales into a list
		size_t num_pairs = m_num_pairs;
		st_table_t buckets = moveBucketsToListLocked(num_pairs + 1);

		// add new pair, if specified
		if (new_bucket)
		{
			buckets.push_back(new_bucket);
			++num_pairs;
		}

		/* TODO
		 * worry about threshold (thus m_table) shrinking and leaking memory.
		 * do we even want threshold to be able to shrink e.g. if we are given
		 * a hint for a large threshold in the constructor?
		 */
		m_threshold = thresholdFromNumPairs(num_pairs);
		m_table.resize(stBucketCountFromThreshold(m_threshold));

		// get balanced hash and hash distribution
		auto hd_pair = findBalancedHashLocked(buckets, m_table.size());
		m_hash = hd_pair.first;
		auto& hash_distribution = hd_pair.second;

		// all subtables should either be empty or null
		for (size_t i = 0; i < m_table.size(); ++i)
		{
			// resize if subtable exists
			if (m_table[i])
				m_table[i]->reserve(hash_distribution[i]);
			// else make a new subtable if needed
			else if (hash_distribution[i])
				m_table[i] = new FastLookupMap<K, V>(hash_distribution[i]);
		}

		// move pairs from list back into subtables
		for (auto& b : buckets) getSubtableLocked(b->first)->insert(b);

		m_num_operations = 0;
		return inserted;
	}

	// move all the nonempty buckets out of the subtables and into one list
	// this places the map in an inconsistent state
	st_table_t moveBucketsToListLocked(size_t size_hint = 0)
	{
		st_table_t buckets;
		buckets.reserve(size_hint);

		for (auto& st_bucket : m_table)
		{
			if (!st_bucket) continue;

			for (auto& bucket : st_bucket->m_table)
			{
				if (!bucket) continue;
				buckets.push_back(bucket);
				bucket = nullptr;
			}

			st_bucket->clear();
		}

		return buckets;
	}

	// calculate a balanced hash onto num_st_buckets for the pairs in buckets.
	// return the hash and the distribution of pairs in the subtables
	// TODO maybe resize m_table in place rather than calculating hash_distribution?
	std::pair<hash_t, std::vector<size_t>> findBalancedHashLocked(const st_table_t& buckets, size_t num_st_buckets) const
	{
		hash_t hash;
		size_t num_buckets;
		std::vector<size_t> hash_distribution(num_st_buckets);

		do
		{
			hash = random_hash<K>(num_st_buckets);

			// Calculate hash distribution
			std::fill(hash_distribution.begin(), hash_distribution.end(), 0);
			for (const auto& b : buckets) ++hash_distribution.at(hash(b->first));

			// Determine number of buckets in resulting subtables
			num_buckets = 0;
			for (auto size : hash_distribution) num_buckets += subtable_t::numBucketsFromNumPairs(size);
		}
		while (!isBucketCountBalancedLocked(num_buckets));

		return std::make_pair(hash, hash_distribution);
	}

	// would the given total number of buckets be balanced for the current threshold?
	bool isBucketCountBalancedLocked(size_t bucket_count) const
	{
		if (bucket_count <= 4 * m_threshold) return true;
		return (bucket_count - 4 * m_threshold) * m_table.size() <= 32 * m_threshold * m_threshold;
	}

	table_t m_table;                // internal hash table
	hash_t m_hash;                  // hash function
	// variables
	size_t m_num_operations; // how many successful inserts/deletes have been performed since the last rebuild
	size_t m_num_pairs; // how many pairs are currently stored
	size_t m_threshold; // M, the threshold
	/* the threshold ties together several aspects of the table:
	 *   - how many operations can be done before a rebuild
	 *   - how many buckets there are at the top level
	 *   - whether the subtables are unbalanced and must be rebuilt
	 */

	mutable boost::upgrade_mutex m_mutex;
	mutable boost::mutex m_st_mutex;
};

#endif
