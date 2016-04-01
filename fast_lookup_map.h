#ifndef FAST_LOOKUP_MAP_H
#define FAST_LOOKUP_MAP_H

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "random_utils.h"

template<class K, class V>
class FastLookupMap
{
	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<K, V> pair_t;
	typedef std::unique_ptr<pair_t> ptr_t;
	typedef std::vector<ptr_t> table_t;

	table_t m_table;    // internal hash table
	hash_t m_hash;      // hash function
	size_t m_capacity;  // how many pairs can be stored without rebuilding
	size_t m_num_pairs; // how many pairs are currently stored

	// convenience functions for getting the unique_ptr for a key
	inline ptr_t& ptr_at(const K& key)
	{
		return m_table.at(m_hash(key));
	}

	inline const ptr_t& ptr_at(const K& key) const
	{
		return m_table.at(m_hash(key));
	}

	// calculate the necessary table size for the current capacity
	inline size_t calculate_table_size() const
	{
		return 2 * m_capacity * (m_capacity - 1);
	}

	// check if the current hash function has no collisions
	bool is_hash_perfect(size_t new_table_size) const
	{
		std::vector<bool> collision_map(new_table_size, false);

		for (const auto& entry_ptr : m_table)
		{
			if (!entry_ptr) continue;

			auto hashed_key = m_hash(entry_ptr->first);
			if (collision_map.at(hashed_key)) return false;

			collision_map[hashed_key] = true;
		}

		return true;
	}

	// rehash each pair in the table (pointless if the hash hasn't changed)
	void rehash_table(size_t new_table_size)
	{
		table_t old_table = std::move(m_table);
		m_table.resize(new_table_size);

		// rehash old pairs
		for (auto& ptr : old_table)
		{
			if (!ptr) continue;

			m_table[m_hash(ptr->first)] = std::move(ptr);
		}
	}

	// reconstruct the table with a new perfect hash function
	// (e.g. if there is a collision or we're over capacity)
	void rebuild_table()
	{
		// if we're over capacity, double it
		while (m_num_pairs > m_capacity) m_capacity *= 2;
		auto new_table_size = calculate_table_size();

		do
		{
			m_hash = random_hash<K>(new_table_size);
		}
		while (!is_hash_perfect(new_table_size));

		rehash_table(new_table_size);
	}

public:
	FastLookupMap(size_t min_capacity = 2)
		: m_num_pairs{ 0 }
	{
		m_capacity = std::max(size_t(2), min_capacity);
		auto new_table_size = calculate_table_size();

		m_table.resize(new_table_size);
		m_hash = random_hash<K>(new_table_size);
	}

	size_t size() const
	{
		return m_num_pairs;
	}

	// try to insert pair into the hash table, rebuilding if necessary
	bool insert(const pair_t& pair)
	{
		// key already exists, do nothing
		if (count(pair.first)) return false;

		++m_num_pairs;
		ptr_t& ptr = ptr_at(pair.first);

		// if we're over capacity or there is a collision
		if (m_num_pairs > m_capacity || ptr)
		{
			// force the new pair into the table and then rebuild it
			m_table.emplace_back(new pair_t(pair));
			rebuild_table();
			return true;
		}

		// no collision, under capacity. simple insert
		ptr = std::make_unique<pair_t>(pair);

		return true;
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		if (!count(key)) return 0;
		--m_num_pairs;
		m_table[m_hash(key)].reset();
		return 1;
	}

	// return the value matching key
	const V& at(const K& key) const
	{
		if (!count(key)) throw std::out_of_range("FastLookupMap::at");
		return ptr_at(key)->second;
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		const ptr_t& ptr = ptr_at(key);
		return ptr && ptr->first == key;
	}

	hash_t hash_function() const
	{
		return m_hash;
	}
};

#endif
