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
public:
	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<const K, V> pair_t;
	typedef std::vector<pair_t*> table_t;

//public:

	FastLookupMap(size_t min_capacity = 2)
		: m_num_pairs {0}
	{
		m_capacity = std::max(size_t(2), min_capacity);
		auto new_table_size = calculateTableSize();

		m_table.resize(new_table_size);
		m_hash = random_hash<K>(new_table_size);
	}

	// Static build of table
	// It is up to the CREATOR of this object to satisfy balancing of sj <= dph_thresh
	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	FastLookupMap(const table_t& buckets)
		: m_num_pairs{ 0 }
	{
		rebuildTable(buckets);
	}

	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	void insert(const table_t& buckets)
	{
		for (pair_t* bucket : buckets)
		{
			insert(bucket);
		}
	}

	bool insert(const pair_t& pair)
	{
		return insert(new pair_t(pair));
	}

	// try to insert pair into the hash table, rebuilding if necessary
	bool insert(pair_t* bucket)
	{
		if (!bucket) return false;

		// key already exists, do nothing
		if (count(bucket->first))
		{
			//throw std::exception();	// FastMap's insert logic shouldn't allow us to get here
			return false;
		}

		++m_num_pairs;

		// if we're over capacity or there is a collision
		if (m_num_pairs > m_capacity || getBucket(bucket->first))
		{
			//throw std::exception();	// FastMap's insert logic shouldn't allow us to get here
			// force the new pair into the table and then rebuild
			m_table.emplace_back(bucket);
			rebuildTable();
			return true;
		}

		// no collision, under capacity. simple insert
		getBucket(bucket->first) = bucket;

		return true;
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		if (!count(key))
		{
			//throw std::exception();	// FastMap's insert logic shouldn't allow us to get here
			return 0;
		}
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

	bool isCollision(const K& key) const
	{
		auto bucket = getBucket(key);
		return bucket && bucket->first != key;
	}

	bool isUnderCapacity() const
	{
		return m_num_pairs < m_capacity;
	}

	size_t size() const
	{
		return m_num_pairs;		// bj
	}

	size_t capacity() const
	{
		return m_capacity;		// mj
	}

	size_t bucketCount() const
	{
		return m_table.size();	// sj, also number of partitions/bucket count
	}

	// how many buckets would there be if we insert another pair?
	size_t bucketCountAfterInsert() const
	{
		auto num_pairs = m_num_pairs + 1;
		auto capacity = m_capacity;
		while (capacity < num_pairs) capacity *= 2;
		return calculateTableSize(capacity);
	}

	hash_t getHash() const
	{
		return m_hash;
	}

// private

	// convenience functions for getting the unique_ptr for a key
	inline pair_t*& getBucket(const K& key)
	{
		return m_table.at(hashKey(m_hash, key));
	}

	inline pair_t* const& getBucket(const K& key) const
	{
		return m_table.at(hashKey(m_hash, key));
	}

	// calculate the necessary table size for the current capacity
	inline size_t calculateTableSize(size_t capacity = m_capacity) const
	{
		return 2 * capacity * (capacity - 1);
	}

	size_t hashKey(hash_t hash, const K& key) const
	{
		return hash(key);
	}

	size_t hashKey(hash_t hash, const pair_t& pair) const
	{
		return hashKey(hash, pair.first);
	}

	size_t hashKey(hash_t hash, const pair_t*& pair) const
	{
		return hashKey(hash, pair->first);
	}

	// check if the current hash function has no collisions
	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
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

	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	hash_t calculateHash(const table_t& buckets, size_t num_buckets)
	{
		hash_t hash;
		do
			hash = random_hash<K>(num_buckets);
		while (!isHashPerfect(buckets, num_buckets, hash));

		return hash;
	}

	// reconstruct the table with a new perfect hash function
	// (e.g. if there is a collision or we're over capacity)
	// TODO seems redundant
	void rebuildTable()
	{
		// if we're over capacity, double it
		while (m_num_pairs > m_capacity) m_capacity *= 2;
		auto new_table_size = calculateTableSize();

		// TODO temporary solution until we can refactor the rebuild
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
		m_table.resize(new_table_size);
		m_hash = calculateHash(buckets, new_table_size);

		// rehash old pairs
		for (auto& bucket : buckets)
		{
			getBucket(bucket->first) = bucket;
		}
	}

	// resize the table to a new size, keeping the same pairs and possibly adding one
	void rebuildTable(size_t new_bucket_count, pair_t* new_bucket = nullptr)
	{
		// TODO assumes that new_bucket_count is sensible
		table_t buckets;
		buckets.reserve(m_num_pairs + 1); // allocate extra in case new_bucket exists

		// TODO why not just copy m_table?
		// Move table contents to list - cannot use FLM iterators as they dereference unique_ptrs
		for (auto& bucket : m_table)
		{
			if (bucket)
			{
				buckets.push_back(bucket);
				bucket = nullptr;
			}
		}
		if (new_bucket)
		{
			buckets.push_back(new_bucket);
			++m_num_pairs;
		}

		m_table.resize(new_bucket_count);
		m_hash = calculateHash(buckets, new_bucket_count);

		// Rehash all elements and place into table
		for (auto& bucket : buckets)
		{
			getBucket(bucket->first) = bucket;
		}
	}

	// replace the contents with those in the given buckets
	void rebuildTable(const table_t& buckets)
	{
		for (auto& bucket : m_table)
		{
			delete bucket;
		}

		m_num_pairs = 0; // don't set equal to distance in case there are duplicates

		// New calculations assume no duplicates, which should be valid if called by FastMap
		auto num_pairs = buckets.size();                       // bj O(n)
		m_capacity = 2 * std::max<size_t>(1, num_pairs);       // mj
		auto bucket_count = calculateTableSize(); // sj

		m_table.resize(bucket_count);

		m_hash = calculateHash(buckets, bucket_count);
		insert(buckets);
	}

	table_t m_table;      // internal hash table
	hash_t m_hash;        // hash function
	size_t m_capacity;    // how many pairs can be stored without rebuilding
	size_t m_num_pairs;   // how many pairs are currently stored
};

#endif
