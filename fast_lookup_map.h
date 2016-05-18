#ifndef FAST_LOOKUP_MAP_H
#define FAST_LOOKUP_MAP_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "rwlock.h"
#include "random_utils.h"

template<class K, class V> class FastMap;

template<class K, class V>
class FastLookupMap
{
	friend FastMap<K,V>;

	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<const K, V> pair_t;
	typedef std::vector<pair_t*> table_t;

	typedef ReadLock read_lock_t;
	typedef WriteLock write_lock_t;
	typedef UpgradeLock readwrite_lock_t;

public:
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

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		// try to bail out early with just a shared lock
		if (!count(key)) return 0;

		// else get a unique lock
		write_lock_t wlock(m_mutex);

		// confirm that the key is really there
		auto& bucket = getBucketLocked(key);
		if (!bucket || bucket->first != key) return 0;

		// remove it
		delete bucket;
		bucket = nullptr;
		--m_num_pairs;

		return 1;
	}

	// return the value matching key
	const V& at(const K& key) const
	{
		read_lock_t rlock(m_mutex);
		auto bucket = getBucketLocked(key);
		if (!bucket || bucket->first != key) throw std::out_of_range("FastLookupMap::at");
		return bucket->second;
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		read_lock_t rlock(m_mutex);
		auto bucket = getBucketLocked(key);
		return bucket && bucket->first == key;
	}

	// return number of pairs
	size_t size() const
	{
		read_lock_t rlock(m_mutex);
		return m_num_pairs; // bj
	}

	// return maximum number of pairs that can be stored without rebuilding
	size_t capacity() const
	{
		read_lock_t rlock(m_mutex);
		return m_capacity; // mj
	}

	// return size of actual hash table
	size_t bucketCount() const
	{
		read_lock_t rlock(m_mutex);
		return m_table.size(); // sj, also number of partitions/bucket count
	}

	// number of elements in given bucket
	size_t bucketSize(size_t n) const
	{
		read_lock_t rlock(m_mutex);
		return n < m_table.size() ? m_table[n] != nullptr : 0;
	}

	// bucket index for key
	size_t bucket(const K& key) const
	{
		read_lock_t rlock(m_mutex);
		return hashKey(m_hash, key);
	}

	// return hash function
	hash_t getHash() const
	{
		read_lock_t rlock(m_mutex);
		return m_hash;
	}

	// reserve enough space for at least num_pairs pairs, possibly rehashing table
	void reserve(size_t num_pairs)
	{
		size_t cap = capacityFromNumPairs(num_pairs);

		read_lock_t rlock(m_mutex);

		if (cap > m_capacity)
		{
			readwrite_lock_t rwlock(rlock);
			m_capacity = cap;
			rebuildLocked();
		}
	}

	// remove all pairs (without actually shrinking table)
	void clear()
	{
		write_lock_t wlock(m_mutex);

		m_num_pairs = 0;
		for (auto& bucket : m_table)
		{
			delete bucket;
			bucket = nullptr;
		}
	}

private:
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

	// convience functions for calculating the hashed value of a key
	static size_t hashKey(const hash_t& hash, const K& key)
	{
		return hash(key);
	}

	// check if a hash function has no collisions for the given buckets
	// (where hash function has range num_buckets)
	static bool isHashPerfect(const table_t& buckets, size_t num_buckets, const hash_t& hash)
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
	static hash_t findCollisionFreeHash(const table_t& buckets, size_t num_buckets)
	{
		hash_t hash;
		do
			hash = random_hash<K>(num_buckets);
		while (!isHashPerfect(buckets, num_buckets, hash));

		return hash;
	}

	// try to insert pair (if non-null), rebuilding if necessary
	bool insert(pair_t* new_bucket)
	{
		// try to bail out early without a unique lock

		// check for null/duplicate
		if (!new_bucket) return false;

		if (count(new_bucket->first))
		{
			delete new_bucket;
			return false;
		}

		write_lock_t wlock(m_mutex);

		// now that we have a unique lock, double check
		auto& bucket = getBucketLocked(new_bucket->first);
		if (bucket && bucket->first == new_bucket->first) return false;

		++m_num_pairs;

		// if we're over capacity or there is a collision
		if (m_num_pairs > m_capacity || bucket)
		{
			// force the new pair into the table and then rebuild
			m_table.push_back(new_bucket);
			rebuildLocked();
		}
		else // no collision, under capacity. simple insert
			bucket = new_bucket;

		return true;
	}

	// check if we can (possibly) insert without rebuilding
	bool isUnderCapacity() const
	{
		read_lock_t rlock(m_mutex);
		return m_num_pairs < m_capacity;
	}

	// convenience functions for getting the bucket for a key
	// XXX m_mutex MUST be owned (shared or unique) by the calling thread
	inline pair_t*& getBucketLocked(const K& key)
	{
		return m_table.at(hashKey(m_hash, key));
	}

	inline pair_t* const& getBucketLocked(const K& key) const
	{
		return m_table.at(hashKey(m_hash, key));
	}

	// how many buckets would there be if we insert another pair?
	size_t bucketCountAfterInsert() const
	{
		read_lock_t rlock(m_mutex);
		auto num_pairs = m_num_pairs + 1;
		auto capacity = m_capacity;
		while (capacity < num_pairs) capacity *= 2;
		return numBucketsFromCapacity(capacity);
	}

	void rebuild()
	{
		write_lock_t wlock(m_mutex);
		rebuildLocked();
	}

	// rebuild the table, getting it back into a consistent state
	// e.g. too many pairs for capacity, collision exists, capacity was changed
	// XXX m_mutex MUST be uniquely owned by the calling thread
	void rebuildLocked()
	{
		// rebuilding is really easy if it's empty
		if (m_num_pairs == 0)
		{
			m_table.resize(numBucketsFromCapacity(m_capacity));
			m_hash = random_hash<K>(m_table.size());
			return;
		}

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

		m_num_pairs = buckets.size();

		// if we're over capacity, double it
		while (m_num_pairs > m_capacity) m_capacity *= 2;
		auto new_table_size = numBucketsFromCapacity(m_capacity);

		// find a new hash function
		m_hash = findCollisionFreeHash(buckets, new_table_size);

		// move pairs back into the hash table
		m_table.resize(new_table_size);
		for (auto& bucket : buckets) getBucketLocked(bucket->first) = bucket;
	}

	table_t m_table;      // internal hash table
	hash_t m_hash;        // hash function
	size_t m_num_pairs;   // how many pairs are currently stored
	size_t m_capacity;    // how many pairs can be stored without rebuilding

	mutable RWMutex m_mutex;
};

#endif
