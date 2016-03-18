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

hash_t random_hash(size_t m)
{
	unsigned int p = random_prime_at_least(m);
	unsigned int a = 0;
	while (a == 0) a = random_uint() % p;
	unsigned int b = random_uint() % p;

	return [m,p,a,b](ktype k) { return ((a * k + b) % p) % m; };
}

class PerfectTable
{
	typedef std::pair<ktype, vtype> pair_t;
	typedef std::unique_ptr<pair_t> ptr_t;
	typedef std::vector<ptr_t> table_t;

	table_t m_table;
	std::vector<bool> m_test_table;
	std::function<size_t(ktype)> m_hash;
	size_t m_capacity;
	size_t m_num_keys;

	// convenience functions for getting the unique_ptr for a key
	inline ptr_t& ptr_at(const ktype& key)
	{
		return m_table.at(m_hash(key));
	}

	inline const ptr_t& ptr_at(const ktype& key) const
	{
		return m_table.at(m_hash(key));
	}

public:
	PerfectTable(const pair_t& pair)
		: m_test_table(1, false),
		m_hash([](ktype) { return 0; })
	{
		m_table.emplace_back(new pair_t(pair));
		m_num_keys = 1;
		m_capacity = m_num_keys * 2;
	}

	// rebuild the hash table with capacity new_capacity and add pair to it
	void rebuild_and_add(size_t new_capacity, const pair_t& pair)
	{
		m_capacity = new_capacity;
		size_t new_size = 2 * m_capacity * (m_capacity - 1);

		m_test_table.resize(new_size);

		// find a perfect hash function for the new size
		while (true)
		{
			bool is_collision_free = true;

			std::fill(m_test_table.begin(), m_test_table.end(), false);
			m_hash = random_hash(new_size);

			m_test_table[m_hash(pair.first)] = true; // guaranteed to be false initially
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

		// new pair
		m_table[m_hash(pair.first)] = std::make_unique<pair_t>(pair);

		// rehash old pairs
		for (auto& ptr : old_table)
		{
			if (!ptr) continue;

			m_table[m_hash(ptr->first)] = std::move(ptr);
		}
	}

	// try to insert pair into the hash table, rebuilding if necessary
	bool insert(const pair_t& pair)
	{
		// can't insert existing key
		if (count(pair.first)) return false;

		++m_num_keys;

		// need to rebuild if this puts us over capacity
		if (m_num_keys > m_capacity)
		{
			rebuild_and_add(m_capacity * 2, pair);
			return true;
		}

		ptr_t& ptr = ptr_at(pair.first);

		// need to rebuild if this causes a collision
		if (!ptr)
		{
			rebuild_and_add(m_capacity, pair);
			return true;
		}

		// under capacity and collision-free, so simply insert
		ptr.reset(new pair_t(pair));

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
		if (!count(key)) throw std::out_of_range("out of range");
		return ptr_at(key)->second;
	}

	// return 1 if pair matching key is in table, else return 0
	int count(const ktype& key) const
	{
		const ptr_t& ptr = ptr_at(key);
		return ptr && ptr->first == key;
	}
};
