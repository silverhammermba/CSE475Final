
#include <functional>
#include <memory>
#include <utility>
#include <vector>

typedef int vtype;
typedef int ktype;
typedef std::function<size_t(ktype)> hash_t;

unsigned int random_prime_at_least(size_t m)
{
	return 2; // TODO probably not greater than m
}

unsigned int random_uint()
{
	return 4; // chosen by fair dice roll.
	          // guaranteed to be random.
}

hash_t gen_random_hash_func(size_t m)
{
	unsigned int p = random_prime_at_least(m);
	unsigned int a = 0;
	while (a == 0) a = random_uint() % p;
	unsigned int b = random_uint() % p;

	return [m,p,a,b](ktype k) { return ((a * k + b) % p) % m; };
}

class PerfectTable
{
public:
	typedef std::pair<ktype, vtype> pair_t;
	typedef std::unique_ptr<pair_t> ptr_t;
	typedef std::vector<ptr_t> table_t;

	table_t m_table;
	std::vector<bool> m_test_table;
	std::function<size_t(ktype)> m_hash;
	size_t m_capacity;
	size_t m_num_keys;

	inline ptr_t& ptr_at(const ktype& key)
	{
		return m_table.at(m_hash(key));
	}

	inline const ptr_t& ptr_at(const ktype& key) const
	{
		return m_table.at(m_hash(key));
	}

//public:
	PerfectTable()
		:m_hash([](ktype) { throw std::exception(); return 0; }), // table and test_table initially size 0 
		m_num_keys(0),
		m_capacity(0)
	{
	}
	PerfectTable(const pair_t& pair)
		: m_test_table(1, false),
		m_hash([](ktype) { return 0; })
	{
		m_table.emplace_back(new pair_t(pair));
		m_num_keys = 1;
		m_capacity = m_num_keys * 2;
	}

	void rebuild_table(size_t new_capacity)
	{
		m_capacity = new_capacity;
		size_t new_size = 2 * m_capacity * (m_capacity - 1);

		m_test_table.resize(new_size);

		while (true)
		{
			bool is_collision_free = true;

			std::fill(m_test_table.begin(), m_test_table.end(), false);
			m_hash = gen_random_hash_func(new_size);

			for (auto& ptr : m_table)
			{
				if (!ptr) continue;

				auto hashed_key = m_hash(ptr->first);
				if (m_test_table.at(hashed_key))	// check if collision with new hash fcn
				{
					is_collision_free = false;
					break;
				}

				m_test_table[hashed_key] = true;
			}

			if (is_collision_free) break;
		}

		// make a new table of size
		table_t old_table = std::move(m_table);
		m_table.resize(new_size);

		// rehash old pairs
		for (auto& ptr : old_table)
		{
			if (!ptr) continue;

			m_table[m_hash(ptr->first)] = std::move(ptr);
		}
	}

	bool insert(const pair_t& pair)
	{
		// key already exists, regardless of value
		if (count(pair.first)) return false;

		++m_num_keys;

		// if we need to grow the table
		if (m_num_keys > m_capacity)
		{
			m_table.emplace_back(new pair_t(pair));
			rebuild_table(m_capacity * 2);
			return true;
		}

		ptr_t& ptr = ptr_at(pair.first);

		// if there is a collision
		if (ptr)
		{
			m_table.emplace_back(new pair_t(pair));
			rebuild_table(m_capacity);
			return true;
		}

		// no collision, no over capacity
		ptr = std::make_unique<pair_t>(pair);

		return true;
	}

	size_t erase(const ktype& key)
	{
		if (!count(key)) return false;
		m_table[m_hash(key)].reset();
		return 1;
	}

	vtype at(const ktype& key) const
	{
		if (!count(key)) throw std::out_of_range("out of range");
		return ptr_at(key)->second;
	}

	int count(const ktype& key) const
	{
		if (m_table.size() == 0) return 0;	// Hash function should be invalid if no table
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