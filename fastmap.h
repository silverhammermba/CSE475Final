#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "random_utils.h"

typedef int vtype;
typedef int ktype;
typedef std::function<size_t(ktype)> hash_t;

hash_t random_hash(size_t m)
{
	unsigned int p = random_prime_at_least(m);
	unsigned int a = random_uint(1, p - 1);
	unsigned int b = random_uint(0, p - 1);

	return [m,p,a,b](ktype k) { return ((a * k + b) % p) % m; };
}

class PerfectTable
{
	typedef std::pair<ktype, vtype> pair_t;
	typedef std::unique_ptr<pair_t> ptr_t;
	typedef std::vector<ptr_t> table_t;

	table_t m_table;                // internal hash table
	std::vector<bool> m_test_table; // table for testing for collision
	hash_t m_hash;                  // hash function
	size_t m_capacity;              // how many pairs can be stored without rebuilding
	size_t m_num_pairs;             // how many pairs are currently stored

	// convenience functions for getting the unique_ptr for a key
	inline ptr_t& ptr_at(const ktype& key)
	{
		return m_table.at(m_hash(key));
	}

	inline const ptr_t& ptr_at(const ktype& key) const
	{
		return m_table.at(m_hash(key));
	}

	// calculate the necessary table size for the current capacity
	inline size_t calculate_size() const
	{
		return 2 * m_capacity * (m_capacity - 1);
	}

	void rebuild_table()
	{
		// if we're over capacity, double it
		while (m_num_pairs > m_capacity) m_capacity *= 2;
		size_t new_size = calculate_size();

		m_test_table.resize(new_size);

		// find a perfect hash function for the new size
		while (true)
		{
			bool is_collision_free = true;

			std::fill(m_test_table.begin(), m_test_table.end(), false);
			m_hash = random_hash(new_size);

			for (auto& ptr : m_table)
			{
				if (!ptr) continue;

				auto hashed_key = m_hash(ptr->first);
				if (m_test_table.at(hashed_key)) // check if collision with new hash fcn
				{
					is_collision_free = false;
					break;
				}

				m_test_table[hashed_key] = true;
			}

			if (is_collision_free) break;
		}

		// rebuild the table with the new hash
		table_t old_table = std::move(m_table);
		m_table.resize(new_size);

		// rehash old pairs
		for (auto& ptr : old_table)
		{
			if (!ptr) continue;

			m_table[m_hash(ptr->first)] = std::move(ptr);
		}
	}

public:
	PerfectTable(size_t min_capacity = 2)
	{
		m_num_pairs = 0;
		m_capacity = std::max(min_capacity, (size_t)2);

		size_t new_size = calculate_size();
		m_table.resize(new_size);
		m_test_table.resize(new_size);
		m_hash = random_hash(new_size);
	}

	size_t size() const
	{
		return m_num_pairs;
	}

	// try to insert pair into the hash table, rebuilding if necessary
	bool insert(const pair_t& pair)
	{
		// key already exists, regardless of value
		if (count(pair.first)) return false;

		++m_num_pairs;

		ptr_t& ptr = ptr_at(pair.first);

		// if we're over capacity or there is a collision, rebuild the table
		if (m_num_pairs > m_capacity || ptr)
		{
			m_table.emplace_back(new pair_t(pair));
			rebuild_table();
			return true;
		}

		// no collision, no over capacity
		ptr = std::make_unique<pair_t>(pair);

		return true;
	}

	// remove pair matching key from the table
	size_t erase(const ktype& key)
	{
		if (!count(key)) return 0;
		m_table[m_hash(key)].reset();
		return 1;
	}

	// return the value matching key
	const vtype& at(const ktype& key) const
	{
		return ptr_at(key)->second;
	}

	// return 1 if pair matching key is in table, else return 0
	int count(const ktype& key) const
	{
		const ptr_t& ptr = ptr_at(key);
		return ptr && ptr->first == key;
	}
};

#if defined(GMOCK_TESTING)

#include "my_gmock.h"

using namespace ::testing;

class APerfectTable : public Test
{
public:
	PerfectTable m_perfect_table;

};

TEST_F(APerfectTable, IsEmptyWhenCreated) {
	ASSERT_THAT(m_perfect_table.m_num_keys, Eq(0u));
	ASSERT_THAT(m_perfect_table.m_table.size(), Eq(0u));
}
TEST_F(APerfectTable, ContainsNoElementThatWasntAdded) {
	ktype k{ 5 };
	ASSERT_THAT(m_perfect_table.count(k), Eq(0u));
	ASSERT_THROW(m_perfect_table.at(k), std::exception);
}

//TEST_F(APerfectTable, ContainsTheElementThatWasAdded) {
//
//	pair_t p(3, 5);
//	ASSERT_THAT(m_perfect_table.count(, Eq(0u));
//}

	// table has no keys at start
	// COUNT a key that doesn't exist returns zero
// COUNT a key that exists returns one
// AT a key that exists returns value
// AT a key that doesn't exist throws
// ERASE a key that exists removes key, decrements num_keys, and returns one
// ERASE a key that doesn't exist does nothing to vector or num_keys, and returns zero
// INSERT duplicate key and same value returns false
// INSERT duplicate key and different value returns false
// Capacity is twice the num keys

#endif
