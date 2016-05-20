#ifndef FAST_MAP_H
#define FAST_MAP_H

#include <unordered_map>
#include <utility>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

template <class K, class V>
class FastMap
{
	typedef boost::shared_lock<boost::shared_mutex> read_lock_t;
	typedef boost::unique_lock<boost::shared_mutex> write_lock_t;
	typedef boost::upgrade_lock<boost::shared_mutex> readwrite_lock_t;

public:
	// construct with a hint that we need to store at least num_pairs pairs
	FastMap(size_t num_pairs = 0)
		: m_table(num_pairs)
	{
	}

	size_t size() const
	{
		read_lock_t lock(m_mutex);
		return m_table.size();
	}

	// try to insert pair into the hash table
	bool insert(const std::pair<const K, V>& pair)
	{
		write_lock_t lock(m_mutex);
		return m_table.insert(pair).second;
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		write_lock_t lock(m_mutex);
		return m_table.erase(key);
	}

	// return the value matching key
	V at(const K& key) const
	{
		read_lock_t lock(m_mutex);
		return m_table.at(key);
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		read_lock_t lock(m_mutex);
		return m_table.count(key);
	}

	// rebuild the entire table
	void rebuild()
	{
	}

private:
	std::unordered_map<K, V> m_table;
	mutable boost::shared_mutex m_mutex;
};

#endif
