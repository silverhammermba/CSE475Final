#ifndef FAST_LOOKUP_MAP_H
#define FAST_LOOKUP_MAP_H

#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "random_utils.h"

template<class K, class V>
class FastLookupMap
{
public:
	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<const K, V> pair_t;
	typedef std::unique_ptr<pair_t> ptr_t;
	typedef std::vector<ptr_t> table_t;

	table_t m_table;    // internal hash table
	hash_t m_hash;      // hash function
	size_t m_capacity;  // how many pairs can be stored without rebuilding
	size_t m_num_pairs; // how many pairs are currently stored

	// convenience functions for getting the unique_ptr for a key
	inline ptr_t& ptr_at(const K& key)
	{
		return m_table.at(hash_key(m_hash, key));
	}

	inline const ptr_t& ptr_at(const K& key) const
	{
		return m_table.at(hash_key(m_hash, key));
	}

	// calculate the necessary table size for the current capacity
	inline size_t calculate_table_size() const
	{
		return 2 * m_capacity * (m_capacity - 1);
	}

	size_t hash_key(hash_t hash, const K& key) const
	{
		return hash(key);
	}
	size_t hash_key(hash_t hash, const pair_t& pair) const
	{
		return hash_key(hash, pair.first);
	}
	size_t hash_key(hash_t hash, const ptr_t& pair) const
	{
		return hash_key(hash, pair->first);
	}

	// check if the current hash function has no collisions
	bool is_hash_perfect(size_t new_table_size) const
	{
		return is_hash_perfect(this->begin(), this->end(), new_table_size, m_hash);
	}

	// check if the current hash function has no collisions
	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	template<class _Iter>
	bool is_hash_perfect(_Iter first, _Iter last, size_t num_buckets, hash_t hash) const
	{
		std::vector<bool> collision_map(num_buckets, false);

		for (; first != last; ++first)
		{
			auto hashed_key = hash_key(hash, *first);
			if (collision_map.at(hashed_key)) return false;

			collision_map[hashed_key] = true;
		}

		return true;
	}

	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	template<class _Iter>
	hash_t calculate_hash(_Iter first, _Iter last, size_t num_buckets)
	{
		hash_t hash;
		do
		{
			hash = random_hash<K>(num_buckets);

		} while (!is_hash_perfect(first, last, num_buckets, hash));

		return hash;
	}

	// rehash each pair in the table (pointless if the hash hasn't changed)
	void rehash_table(size_t new_table_size)
	{
		table_t old_table = std::move(m_table);
		m_table.resize(new_table_size);

		// rehash old pairs
		for (auto& ptr : old_table)
		{
			if (!ptr) continue;

			m_table.at(hash_key(m_hash, ptr)) = std::move(ptr);
		}
	}

	// reconstruct the table with a new perfect hash function
	// (e.g. if there is a collision or we're over capacity)
	void rebuild_table()
	{
		// if we're over capacity, double it
		while (m_num_pairs > m_capacity) m_capacity *= 2;
		auto new_table_size = calculate_table_size();

		do
		{
			m_hash = random_hash<K>(new_table_size);
		} while (!is_hash_perfect(new_table_size));

		rehash_table(new_table_size);
	}

	void rebuild_table(size_t new_table_size, ptr_t new_element = ptr_t())
	{
		std::vector<ptr_t> element_list;
		element_list.reserve(m_num_pairs + 1);
		m_table.resize(new_table_size);

		// Move table contents to list - cannot use iterators as they dereference unique_ptrs
		for (auto& pair : m_table)
		{
			if (pair != nullptr) element_list.emplace_back(std::move(pair));
		}
		if (new_element != nullptr)
		{
			element_list.emplace_back(std::move(new_element));
			++m_num_pairs;
		}

		m_hash = calculate_hash(element_list.begin(), element_list.end(), new_table_size);

		for (auto it = element_list.begin(); it != element_list.end(); ++it)
		{
			m_table.at(hash_key(m_hash, *it)) = std::move(*it);
		}
	}

	template <class T>
	class ForwardIterator
		: public std::iterator<std::forward_iterator_tag, pair_t>
	{
		// needs to call explicit constructor
		friend FastLookupMap;

		// the const here only protects the unique_ptrs, so it works even for the non-const iterator
		typedef typename table_t::const_iterator itr_t;

		itr_t it, end;

		// construct from hash table iterator
		explicit ForwardIterator(const itr_t& _it, const itr_t& _end)
			: it{ _it }, end{ _end }
		{
			while (it != end && !*it) ++it;
		}

	public:
		ForwardIterator()
		{
		}

		ForwardIterator(const ForwardIterator& other)
			: it{ other.it }, end{ other.end }
		{
		}

		ForwardIterator& operator=(const ForwardIterator& other)
		{
			it = other.it;
			end = other.end;
			return *this;
		}

		void swap(ForwardIterator& other) NOEXCEPT
		{
			std::swap(it, other.it);
			std::swap(end, other.end);
		}

		// ++it
		const ForwardIterator& operator++()
		{
			do
			{
				++it;
			} while (it != end && !*it);

			return *this;
		}

		// it++
		ForwardIterator operator++(int)
		{
			ForwardIterator prev{ it, end };

			do
			{
				++it;
			} while (it != end && !*it);

			return prev;
		}

		template<class OtherType>
		bool operator==(const ForwardIterator<OtherType>& other) const
		{
			return it == other.it && end == other.end;
		}

		template<class OtherType>
		bool operator!=(const ForwardIterator<OtherType>& other) const
		{
			return it != other.it || end != other.end;
		}

		T& operator*() const
		{
			return **it;
		}

		T* operator->() const
		{
			return &(**it);
		}
	};

//public:

	typedef ForwardIterator<pair_t> iterator;
	typedef ForwardIterator<const pair_t> const_iterator;

	FastLookupMap(size_t min_capacity = 2)
		: m_num_pairs{ 0 }
	{
		m_capacity = std::max(size_t(2), min_capacity);
		auto new_table_size = calculate_table_size();

		m_table.resize(new_table_size);
		m_hash = random_hash<K>(new_table_size);
	}

	// Static build of table
	// It is up to the CREATOR of this object to satisfy balancing of sj <= dph_thresh
	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	template <class Iter>
	FastLookupMap(Iter first, Iter last)
	{
		auto num_values = std::distance(first, last);			// bj O(n)
		m_capacity = 2 * std::max<size_t>(1, num_values);		// mj
		auto table_size = 2 * m_capacity * (m_capacity - 1);	// sj

		m_table.resize(table_size);

		m_hash = calculate_hash(first, last, table_size);
		insert(first, last);
	}

	size_t size() const
	{
		return m_num_pairs;
	}

	size_t capacity() const
	{
		return m_capacity;
	}

	size_t allocated() const
	{
		return m_table.size();
	}

	// try to insert pair into the hash table, rebuilding if necessary
	bool insert(const pair_t& pair)
	{
		auto u_pair = std::make_unique<pair_t>(pair);

		return insert(std::move(u_pair));
	}

	// try to insert pair into the hash table, rebuilding if necessary
	bool insert(ptr_t pair)
	{
		// key already exists, do nothing
		if (count(pair->first)) return false;

		++m_num_pairs;
		ptr_t& ptr = ptr_at(pair->first);

		// if we're over capacity or there is a collision
		if (m_num_pairs > m_capacity || ptr)
		{
			// force the new pair into the table and then rebuild
			m_table.emplace_back(std::move(pair));
			rebuild_table();
			return true;
		}

		// no collision, under capacity. simple insert
		ptr = std::move(pair);

		return true;
	}

	// try to insert pairs into the hash table, rebuilding if necessary
	// Arguments: iterators to a vector of pair<K,V> or [unique]pointers to pair<K,V>
	template<class Iter>
	void insert(Iter first, Iter last)
	{
		for (; first != last; ++first)
		{
			insert(std::move(*first));
		}
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		if (!count(key)) return 0;
		--m_num_pairs;
		m_table[m_hash(key)].reset();
		return 1;
	}

	// return the value matching key
	const V& at(const K& key) const
	{
		if (!count(key)) throw std::out_of_range("FastLookupMap::at");
		return ptr_at(key)->second;
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		const ptr_t& ptr = ptr_at(key);
		return ptr && ptr->first == key;
	}

	hash_t hash_function() const
	{
		return m_hash;
	}

	iterator begin()
	{
		return iterator(m_table.cbegin(), m_table.cend());
	}

	iterator end()
	{
		return iterator(m_table.cend(), m_table.cend());
	}

	const_iterator cbegin() const
	{
		return const_iterator(m_table.cbegin(), m_table.cend());
	}

	const_iterator cend() const
	{
		return const_iterator(m_table.cend(), m_table.cend());
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator end() const
	{
		return cend();
	}
};

#endif
