#ifndef FAST_LOOKUP_MAP_H
#define FAST_LOOKUP_MAP_H

#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "random_utils.h"

template<class K, class V>
class FastLookupMap
{
	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<const K, V> pair_t;
	typedef std::vector<pair_t*> table_t;

public:
	// static functions for determining hash table sizes

	// how big would a hash table be with the given capacity?
	static size_t numBucketsFromCapacity(size_t capacity)
	{
		return 2 * capacity * (capacity - 1);
	}

	// how much capacity should we have if we know we need to store num_pairs?
	static size_t capacityFromNumPairs(size_t num_pairs)
	{
		return 2 * std::max<size_t>(1, num_pairs);
	}

	// how big would a hash table be if initialized with the given number of pairs?
	static size_t numBucketsFromNumPairs(size_t num_pairs)
	{
		return numBucketsFromCapacity(capacityFromNumPairs(num_pairs));
	}

	// construct with a hint that we need to store at least num_pairs pairs
	FastLookupMap(size_t num_pairs = 0)
		: m_num_pairs {0},
		m_capacity {capacityFromNumPairs(num_pairs)}
	{
		rebuild();
	}

	~FastLookupMap()
	{
		for (auto& bucket : m_table) delete bucket;
	}

	// try to insert a pair
	bool insert(const pair_t& pair)
	{
		return insert(new pair_t(pair));
	}

	// try to insert pair (if non-null), rebuilding if necessary
	bool insert(pair_t* bucket)
	{
		// check for null/duplicate
		if (!bucket || count(bucket->first)) return false;

		++m_num_pairs;

		// if we're over capacity or there is a collision
		if (m_num_pairs > m_capacity || getBucket(bucket->first))
		{
			// force the new pair into the table and then rebuild
			m_table.push_back(bucket);
			rebuild();
			return true;
		}

		// no collision, under capacity. simple insert
		getBucket(bucket->first) = bucket;

		return true;
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		if (!count(key)) return 0;

		--m_num_pairs;
		delete getBucket(key);
		getBucket(key) = nullptr;

		return 1;
	}

	// return the value matching key
	const V& at(const K& key) const
	{
		if (!count(key)) throw std::out_of_range("FastLookupMap::at");
		return getBucket(key)->second;
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		auto bucket = getBucket(key);
		return bucket && bucket->first == key;
	}

	// check if we can (possibly) insert without rebuilding
	bool isUnderCapacity() const
	{
		return m_num_pairs < m_capacity;
	}

	// return number of pairs
	size_t size() const
	{
		return m_num_pairs; // bj
	}

	// return maximum number of pairs that can be stored without rebuilding
	size_t capacity() const
	{
		return m_capacity; // mj
	}

	// return size of actual hash table
	size_t bucketCount() const
	{
		return m_table.size(); // sj, also number of partitions/bucket count
	}

	// how many buckets would there be if we insert another pair?
	size_t bucketCountAfterInsert() const
	{
		auto num_pairs = m_num_pairs + 1;
		auto capacity = m_capacity;
		while (capacity < num_pairs) capacity *= 2;
		return numBucketsFromCapacity(capacity);
	}

	// return hash function
	hash_t getHash() const
	{
		return m_hash;
	}

	// convenience functions for getting the unique_ptr for a key
	inline pair_t*& getBucket(const K& key)
	{
		return m_table.at(hashKey(m_hash, key));
	}

	inline pair_t* const& getBucket(const K& key) const
	{
		return m_table.at(hashKey(m_hash, key));
	}

	// convience functions for calculating the hashed value of a key
	size_t hashKey(hash_t hash, const K& key) const
	{
		return hash(key);
	}

	size_t hashKey(hash_t hash, const pair_t& pair) const
	{
		return hashKey(hash, pair.first);
	}

	size_t hashKey(hash_t hash, pair_t* const& pair) const
	{
		return hashKey(hash, pair->first);
	}

	// check if a hash function has no collisions for the given buckets
	// (where hash function has range num_buckets)
	bool isHashPerfect(const table_t& buckets, size_t num_buckets, const hash_t& hash) const
	{
		std::vector<bool> collision_map(num_buckets, false);

		for (auto& bucket : buckets)
		{
			if (!bucket) continue;
			auto hashed_key = hashKey(hash, bucket->first);
			if (collision_map.at(hashed_key)) return false;

			collision_map[hashed_key] = true;
		}

		return true;
	}

	// find a collision-free hash function for the given buckets
	// (where hash function has range num_buckets)
	hash_t findCollisionFreeHash(const table_t& buckets, size_t num_buckets)
	{
		hash_t hash;
		do
			hash = random_hash<K>(num_buckets);
		while (!isHashPerfect(buckets, num_buckets, hash));

		return hash;
	}

	// rebuild the table, getting it back into a consistent state
	// e.g. too many pairs for capacity, collision exists, capacity was changed
	void rebuild()
	{
		// rebuilding is really easy if it's empty
		if (m_num_pairs == 0)
		{
			m_table.resize(numBucketsFromCapacity(m_capacity));
			m_hash = random_hash<K>(m_table.size());
			return;
		}

		// if we're over capacity, double it
		while (m_num_pairs > m_capacity) m_capacity *= 2;
		auto new_table_size = numBucketsFromCapacity(m_capacity);

		// move all pairs into a temporary vector
		table_t buckets;
		buckets.reserve(m_num_pairs);
		for (auto& bucket : m_table)
		{
			if (bucket)
			{
				buckets.push_back(bucket);
				bucket = nullptr;
			}
		}

		// find a new hash function
		m_hash = findCollisionFreeHash(buckets, new_table_size);

		// move pairs back into the hash table
		m_table.resize(new_table_size);
		for (auto& bucket : buckets) getBucket(bucket->first) = bucket;
	}

	// remove all pairs (without actually shrinking table)
	void clear()
	{
		m_num_pairs = 0;
		for (auto& bucket : m_table)
		{
			delete bucket;
			bucket = nullptr;
		}
	}

	// resize the table assuming that we need to store num_pairs (only grows, doesn't shrink)
	void resize(size_t num_pairs)
	{
		size_t cap = capacityFromNumPairs(num_pairs);
		if (cap > m_capacity)
		{
			m_capacity = cap;
			rebuild();
		}
	}

	table_t m_table;      // internal hash table
	hash_t m_hash;        // hash function
	size_t m_num_pairs;   // how many pairs are currently stored
	size_t m_capacity;    // how many pairs can be stored without rebuilding
};

#endif
