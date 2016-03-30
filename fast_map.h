
#include "perfect_table.h"
#include "gtest_include.h"

template <class K, class V>
class FastMap
{
	typedef std::unique_ptr<PerfectTable<K, V>> ptr_t;
	typedef std::vector<ptr_t> table_t;
public:
	FastMap()
	{

	}
	// try to insert pair into the hash table
	bool insert(const pair_t& pair)
	{
		auto& ptr = ptr_at(pair.first);
		if (!ptr) ptr_at(pair.first) = std::make_unique<PerfectTable>();
		return ptr->insert(pair);
	}

	// remove pair matching key from the table
	size_t erase(const ktype& key)
	{
		auto& ptr = ptr_at(key);
		return !ptr ? 0 : ptr->erase(key);
	}

	// return the value matching key
	vtype at(const ktype& key) const
	{
		auto& ptr = ptr_at(key);
		return !ptr ? 0 : ptr->at(key);
	}

	// return 1 if pair matching key is in table, else return 0
	int count(const ktype& key) const
	{
		auto& ptr = ptr_at(key);
		return !ptr ? 0 : ptr->count(key);
	}

private:
	// convenience functions for getting the unique_ptr for a key
	inline ptr_t& ptr_at(const ktype& key)
	{
		return m_table.at(m_hash(key));
	}

	inline const ptr_t& ptr_at(const ktype& key) const
	{
		return m_table.at(m_hash(key));
	}

	table_t m_table;                // internal hash table
	hash_t m_hash;                  // hash function

};