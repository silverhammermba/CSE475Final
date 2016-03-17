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

class STable
{
	typedef std::pair<ktype,vtype> pair_t;
	typedef std::unique_ptr<pair_t> ptr_t;
	typedef std::vector<ptr_t> table_t;

	table_t table;
	std::vector<bool> test_table;
	std::function<size_t(ktype)> hash;
	size_t num_keys;

	inline ptr_t& ptr_at(const ktype& key)
	{
		return table.at(hash(key));
	}

	inline const ptr_t& ptr_at(const ktype& key) const
	{
		return table.at(hash(key));
	}

public:
	STable(const pair_t& pair)
		: test_table(1, false), 
		hash([](ktype) { return 0; })
	{
		table.emplace_back(new pair_t(pair));
		num_keys = 1;
	}

	bool insert(const pair_t& pair)
	{
		// already exists
		if (count(pair.first)) return false;

		ptr_t& ptr = ptr_at(pair.first);
		++num_keys;

		// collision
		if (ptr)
		{
			// TODO rebuild table
			auto table_size = num_keys * num_keys;
			test_table.resize(table_size);

			while (true)
			{
				bool is_collision_free = true;
				
				std::fill(test_table.begin(), test_table.end(), false);
				hash = gen_random_hash_func(table_size);
				
				test_table[hash(pair.first)] = true;	// guaranteed to be false initially
				for (auto& ptr : table)
				{
					if (!ptr) continue;
					auto hashed_key = hash(ptr->first);
					if (test_table.at(hashed_key))	// check if collision with new hash fcn
					{
						is_collision_free = false;
						break;
					}
					else
					{
						test_table[hashed_key] = true;
					}
				}

				if (is_collision_free) break;
			}

			// rehash table using moves
			// hash new key and add

			return true;
		}

		// easy insert
		ptr.reset(new pair_t(pair));
		return true;
	}

	size_t erase(const ktype& key)
	{
		if (!count(key)) return 0;
		table[hash(key)].reset();
		return 1;
	}

	vtype at(const ktype& key) const
	{
		if (!count(key)) throw std::out_of_range("out of range");
		return ptr_at(key)->second;
	}

	int count(const ktype& key) const
	{
		const ptr_t& ptr = ptr_at(key);
		return ptr && ptr->first == key;
	}
};
