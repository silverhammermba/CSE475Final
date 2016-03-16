#include <functional>
#include <memory>
#include <utility>
#include <vector>

typedef int vtype;
typedef int ktype;

typedef std::function<size_t(ktype)> hash_t;

hash_t gen_random_hash_func(size_t m)
{
	unsigned int p = random_prime_greater_than(m);
	unsigned int a = 0;
	while (a == 0) a = random_int() % p;
	unsigned int b = random_int() % p;

	return [m,p,a,b](ktype k) { return ((a * k + b) % p) % m; };
}

class STable
{
	typedef std::pair<ktype,vtype> pair_t;
	typedef std::unique_ptr<pair_t> ptr_t;
	typedef std::vector<ptr_t> table_t;

	std::function<size_t(ktype)> hash;
	table_t table;
	std::vector<bool> test_table;
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
	STable(vtype v)
		: table(1, v), test_table(1, false), hash([](ktype k) { return 0; })
	{
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
			test_table.resize(num_keys * num_keys);

			while (true)
			{
				// reset test_table to falses

			}

			return true;
		}

		// easy insert
		ptr.reset(new std::pair<ktype, vtype>(pair));
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
