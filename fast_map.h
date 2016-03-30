
#include "gtest_include.h"
#include "perfect_table.h"

template <class K, class V>
class FastMap
{
	typedef std::function<size_t(K)> hash_t;
	typedef std::unique_ptr<PerfectTable<K, V>> ptr_t;
	typedef std::pair<K, V> pair_t;
	typedef std::vector<ptr_t> table_t;
public:
	FastMap(size_t num_buckets = 2)
		:m_num_pairs{ 0 }
	{
		if (num_buckets == 0) throw std::out_of_range("FastMap num_buckets must not be zero");
		m_table.resize(num_buckets);
		m_hash = random_hash<K>(unsigned int(num_buckets));
	}	
	size_t size() const
	{
		return m_num_pairs;
	}
	// try to insert pair into the hash table
	bool insert(const pair_t& pair)
	{
		auto& ptr = ptr_at(pair.first);
		if (!ptr) ptr_at(pair.first) = std::make_unique<PerfectTable<K,V>>();
		auto ret = ptr->insert(pair);
		if (ret) ++m_num_pairs;
		return ret;
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		auto& ptr = ptr_at(key);
		auto ret = !ptr ? 0 : ptr->erase(key);
		if (ret) --m_num_pairs;
		return ret;
	}

	// return the value matching key
	V at(const K& key) const
	{
		if (!count(key)) throw std::out_of_range("FastMap::at");
		auto& ptr = ptr_at(key);
		return !ptr ? 0 : ptr->at(key);
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		auto& ptr = ptr_at(key);
		return !ptr ? 0 : ptr->count(key);
	}

private:
	// convenience functions for getting the unique_ptr for a key
	inline ptr_t& ptr_at(const K& key)
	{
		return m_table.at(m_hash(key));
	}

	inline const ptr_t& ptr_at(const K& key) const
	{
		return m_table.at(m_hash(key));
	}

	table_t m_table;                // internal hash table
	hash_t m_hash;                  // hash function
	size_t m_num_pairs;				// how many pairs are currently stored
};