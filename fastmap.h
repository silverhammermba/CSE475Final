typedef int vtype;
typedef int ktype;

typedef std::function<size_t(ktype)> hash_t;

hash_t gen_random_hash_func(size_t m)
{
	int p = random_prime_greater_than(m);
	int a;
	do {
		a = random_int() % p;
	} while(a == 0)
	b = random_int() % p;
	return [m,p,a,b](ktype k) { return ((a * k + b) % p) % m; };
}

class FastMap
{
	class STable
	{
		typedef std::vector<std::unique_ptr<vtype>> table;

		std::function<size_t(ktype)> hash;
		table m_table;

	public:
		STable(vtype v)
			: m_table(1, v), hash([](ktype k) { return 0; })
		{
		}
	};

	typedef std::vector<std::unique_ptr<s_table>> table;

	table m_table;

	const int default_min_bucket_count = 10;

public:
	FastMap(size_t min_bucket_count = default_min_bucket_count)
		: m_table (min_bucket_count == 0 ? default_min_bucket_count : min_bucket_count)
	{
	}

	~FastMap()
	{
	}

	int size() const
	{
	}

	void insert(ktype key, vtype value)
	{
	}

	void erase(ktype key)
	{
		func = RandomHash
	}

	vtype at(const ktype key) const
	{
	}

	int count(const ktype key) const
	{
	}
};
