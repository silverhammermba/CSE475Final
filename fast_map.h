#ifndef FAST_MAP_H
#define FAST_MAP_H

#include <mutex>
#include <unordered_map>
#include <utility>

template <class K, class V>
class FastMap
{
public:
	// construct with a hint that we need to store at least num_pairs pairs
	FastMap(size_t num_pairs = 0)
		: m_table{num_pairs}
	{
	}

	size_t size() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_table.size();
	}

	// try to insert pair into the hash table
	bool insert(const std::pair<const K, V>& pair)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_table.insert(pair).second;
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_table.erase(key);
	}

	// return the value matching key
	V at(const K& key) const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_table.at(key);
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_table.count(key);
	}

	// rebuild the entire table
	void rebuild()
	{
	}

private:
	std::unordered_map<K, V> m_table;
	mutable std::mutex m_mutex;
};

#endif
