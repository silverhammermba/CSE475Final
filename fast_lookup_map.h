#ifndef FAST_LOOKUP_MAP_H
#define FAST_LOOKUP_MAP_H

#include <atomic>
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
	typedef std::unique_ptr<pair_t> upair_t;
	typedef std::vector<upair_t> upair_list_t;
	typedef std::vector<upair_t> table_t;

	template <class T>
	class ForwardIterator
		: public std::iterator<std::forward_iterator_tag, pair_t>
	{
		// needs to call explicit constructor
		friend FastLookupMap;

		// the const here only protects the unique_ptrs, so it works even for the non-const iterator
		typedef typename table_t::const_iterator itr_t;

		itr_t it, end;

		// construct from hash table iterator
		explicit ForwardIterator(const itr_t& _it, const itr_t& _end)
			: it{ _it }, end{ _end }
		{
			while (it != end && !*it) ++it;
		}

	public:
		ForwardIterator()
		{
		}

		ForwardIterator(const ForwardIterator& other)
			: it{ other.it }, end{ other.end }
		{
		}

		ForwardIterator& operator=(const ForwardIterator& other)
		{
			it = other.it;
			end = other.end;
			return *this;
		}

		void swap(ForwardIterator& other) NOEXCEPT
		{
			std::swap(it, other.it);
			std::swap(end, other.end);
		}

		// ++it
		const ForwardIterator& operator++()
		{
			do
			{
				++it;
			} while (it != end && !*it);

			return *this;
		}

		// it++
		ForwardIterator operator++(int)
		{
			ForwardIterator prev{ it, end };

			do
			{
				++it;
			} while (it != end && !*it);

			return prev;
		}

		template<class OtherType>
		bool operator==(const ForwardIterator<OtherType>& other) const
		{
			return it == other.it && end == other.end;
		}

		template<class OtherType>
		bool operator!=(const ForwardIterator<OtherType>& other) const
		{
			return it != other.it || end != other.end;
		}

		T& operator*() const
		{
			return **it;
		}

		T* operator->() const
		{
			return &(**it);
		}
	};

	typedef ForwardIterator<pair_t> iterator;
	typedef ForwardIterator<const pair_t> const_iterator;

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
	template <class Iter>
	FastLookupMap(Iter first, Iter last)
		: m_num_pairs{ 0 }
	{
		rebuildTable(first, last);
	}

	// try to insert pair into the hash table, rebuilding if necessary
	bool insert(const pair_t& pair)
	{
		auto u_pair = std::make_unique<pair_t>(pair);

		return insert(std::move(u_pair));
	}

	// try to insert pairs into the hash table, rebuilding if necessary
	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	template<class Iter>
	void insert(Iter first, Iter last)
	{
		for (; first != last; ++first)
		{
			insert(std::move(*first));
		}
	}

	// try to insert pair into the hash table, rebuilding if necessary
	bool insert(upair_t pair)
	{
		// key already exists, do nothing
		if (count(pair->first))
		{
			//throw std::exception();	// FastMap's insert logic shouldn't allow us to get here
			return false;
		}

		++m_num_pairs;
		upair_t& bucket = getBucket(pair->first);

		// if we're over capacity or there is a collision
		if (m_num_pairs > m_capacity || bucket)
		{
			//throw std::exception();	// FastMap's insert logic shouldn't allow us to get here
			// force the new pair into the table and then rebuild
			m_table.emplace_back(std::move(pair));
			rebuildTable();
			return true;
		}

		// no collision, under capacity. simple insert
		bucket = std::move(pair);

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
		getBucket(key).reset();
		return 1;
	}

	// return the value matching key
	const V& at(const K& key)
	{
		if (!count(key)) throw std::out_of_range("FastLookupMap::at");
		return getBucket(key)->second;
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		const upair_t& bucket = getBucket(key);
		return bucket && bucket->first == key;
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

// private

	hash_t getHash() const
	{
		return m_hash;
	}
	
	bool isCollision(const K& key) const
	{
		const upair_t& bucket = getBucket(key);
		return bucket && bucket->first != key;
	}

	// convenience functions for getting the unique_ptr for a key
	inline upair_t& getBucket(const K& key)
	{
		return m_table.at(hashKey(m_hash, key));
	}

	inline const upair_t& getBucket(const K& key) const
	{
		return m_table.at(hashKey(m_hash, key));
	}

	// calculate the necessary table size for the current capacity
	inline size_t calculateTableSize() const
	{
		return 2 * m_capacity * (m_capacity - 1);
	}

	size_t hashKey(hash_t hash, const K& key) const
	{
		return hash(key);
	}

	size_t hashKey(hash_t hash, const pair_t& pair) const
	{
		return hashKey(hash, pair.first);
	}

	size_t hashKey(hash_t hash, const upair_t& pair) const
	{
		return hashKey(hash, pair->first);
	}

	// check if the current hash function has no collisions
	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	template<class Iter>
	bool isHashPerfect(Iter first, Iter last, size_t num_buckets, hash_t hash) const
	{
		std::vector<bool> collision_map(num_buckets, false);

		for (; first != last; ++first)
		{
			auto hashed_key = hashKey(hash, *first);
			if (collision_map.at(hashed_key)) return false;

			collision_map[hashed_key] = true;
		}

		return true;
	}

	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	template<class Iter>
	hash_t calculateHash(Iter first, Iter last, size_t num_buckets)
	{
		hash_t hash;
		do
			hash = random_hash<K>(num_buckets);
		while (!isHashPerfect(first, last, num_buckets, hash));

		return hash;
	}

	// rehash each pair in the table (pointless if the hash hasn't changed)
	void rehashTable(size_t new_bucket_count)
	{
		table_t old_table = std::move(m_table);
		m_table.resize(new_bucket_count);

		// rehash old pairs
		for (auto& bucket : old_table)
		{
			if (!bucket) continue;

			m_table.at(hashKey(m_hash, bucket)) = std::move(bucket);
		}
	}

	// reconstruct the table with a new perfect hash function
	// (e.g. if there is a collision or we're over capacity)
	void rebuildTable()
	{
		// if we're over capacity, double it
		while (m_num_pairs > m_capacity) m_capacity = m_capacity * 2;
		auto new_table_size = calculateTableSize();

		m_hash = calculateHash(this->begin(), this->end(), new_table_size);

		rehashTable(new_table_size);
	}

	void rebuildTable(size_t new_bucket_count, upair_t new_upair = upair_t())
	{
		upair_list_t upair_list;
		upair_list.reserve(m_num_pairs + 1); // allocate extra in case new_upair exists
		m_table.resize(new_bucket_count);

		// Move table contents to list - cannot use FLM iterators as they dereference unique_ptrs
		for (auto& upair : m_table)
		{
			if (upair != nullptr) upair_list.emplace_back(std::move(upair));
		}
		if (new_upair != nullptr)
		{
			upair_list.emplace_back(std::move(new_upair));
			++m_num_pairs;
		}

		m_hash = calculateHash(upair_list.begin(), upair_list.end(), new_bucket_count);

		// Rehash all elements and place into table
		for (auto it = upair_list.begin(); it != upair_list.end(); ++it)
		{
			getBucket(it->get()->first) = std::move(*it);
		}
	}

	template<class Iter>
	void rebuildTable(Iter first, Iter last)
	{
		for (auto& upair : m_table)
		{
			upair.reset();
		}

		m_num_pairs = 0;	// don't set equal to distance in case there are duplicates

		// New calculations assume no duplicates, which should be valid if called by FastMap
		auto num_pairs = std::distance(first, last);			// bj O(n)
		m_capacity = 2 * std::max<size_t>(1, num_pairs);		// mj
		auto bucket_count = 2 * m_capacity * (m_capacity - 1);	// sj

		m_table.resize(bucket_count);

		m_hash = calculateHash(first, last, bucket_count);
		insert(first, last);
	}

	iterator begin()
	{
		return iterator(m_table.cbegin(), m_table.cend());
	}

	iterator end()
	{
		return iterator(m_table.cend(), m_table.cend());
	}

	const_iterator cbegin() const
	{
		return const_iterator(m_table.cbegin(), m_table.cend());
	}

	const_iterator cend() const
	{
		return const_iterator(m_table.cend(), m_table.cend());
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator end() const
	{
		return cend();
	}

	table_t m_table;      // internal hash table
	hash_t m_hash;        // hash function
	size_t m_capacity;    // how many pairs can be stored without rebuilding
	size_t m_num_pairs;   // how many pairs are currently stored
};

#endif
