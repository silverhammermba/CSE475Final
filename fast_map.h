#ifndef FAST_MAP_H
#define FAST_MAP_H

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include "fast_lookup_map.h"

template <class K, class V>
class FastMap
{
public:
	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<const K, V> pair_t;
	typedef FastLookupMap<K, V> subtable_t;
	typedef std::vector<subtable_t*> table_t;
	typedef std::vector<pair_t*> bucket_list_t;
	typedef std::vector<std::vector<pair_t*>> hashed_bucket_list_t;

	template <class T>
	class ForwardIterator
		: public std::iterator<std::forward_iterator_tag, pair_t>
	{
		friend FastMap; // needs to call explicit constructor

		typedef typename table_t::const_iterator o_itr_t;
		typedef typename FastLookupMap<K, V>::iterator i_itr_t;

		o_itr_t outer_it, outer_end;
		i_itr_t inner_it;

		// check if outer_it/inner_it are in a valid state
		inline bool isValid() const
		{
			// either outer_it as at the end or it is pointing to a nonempty FastLookupMap and inner_it is pointing to a pair
			return outer_it == outer_end || (*outer_it && inner_it != (*outer_it)->end());
		}

		// check if outer_it can be used to get a FastLookupMap
		inline bool outerCanDeref() const
		{
			return outer_it != outer_end && *outer_it;
		}

		// ensure we are in a valid state (by updating outer_it/inner_it)
		inline void makeValid()
		{
			while (!isValid())
			{
				++outer_it; // go to the next bucket
				if (outerCanDeref()) inner_it = (*outer_it)->begin();
			}
		}

		// construct from hash table iterator
		explicit ForwardIterator(const o_itr_t& _outer_it, const o_itr_t& _outer_end)
			: outer_it{ _outer_it }, outer_end{ _outer_end }
		{
			if (outerCanDeref()) inner_it = (*outer_it)->begin();
			makeValid();
		}

	public:
		ForwardIterator()
		{
		}

		ForwardIterator(const ForwardIterator& other)
			: outer_it{ other.outer_it }, outer_end{ other.outer_end }, inner_it{ other.inner_it }
		{
		}

		ForwardIterator& operator=(const ForwardIterator& other)
		{
			outer_it = other.outer_it;
			outer_end = other.outer_end;
			inner_it = other.inner_it;
			return *this;
		}

		void swap(ForwardIterator& other) NOEXCEPT
		{
			std::swap(outer_it, other.outer_it);
			std::swap(outer_end, other.outer_end);
			std::swap(inner_it, other.inner_it);
		}

		// ++it
		const ForwardIterator& operator++()
		{
			++inner_it;
			makeValid();

			return *this;
		}

		// it++
		ForwardIterator operator++(int)
		{
			ForwardIterator prev{ outer_it, outer_end, inner_it };

			++inner_it;
			makeValid();

			return prev;
		}

		template<class OtherType>
		bool operator==(const ForwardIterator<OtherType>& other) const
		{
			return outer_it == other.outer_it && outer_end == other.outer_end && (outer_it == outer_end || inner_it == other.inner_it);
		}

		template<class OtherType>
		bool operator!=(const ForwardIterator<OtherType>& other) const
		{
			return outer_it != other.outer_it || outer_end != other.outer_end || (outer_it != outer_end && inner_it != other.inner_it);
		}

		T& operator*() const
		{
			return *inner_it;
		}

		T* operator->() const
		{
			return &(*inner_it);
		}
	};
//public:

	typedef ForwardIterator<pair_t> iterator;
	typedef ForwardIterator<const pair_t> const_iterator;

	FastMap()
		: m_C(2),
		m_SM_SCALING(1),
		m_num_pairs(0),
		m_num_operations(0)
	{
		// Could call fullRehash here instead to reuse code
		// Performed here now, otherwise a fullRehash would occur and do same operation after first insert
		m_capacity = (1 + m_C) * std::max(this->size(), size_t(4));	// Calculate new element count threshold
		auto bucket_count = s(m_capacity);							// Calculate new number of partitions/buckets in Top Level Table
		m_table.resize(bucket_count);								// Grow table if needed to accomodate new number of partitions/buckets
		m_hash = random_hash<K>(bucket_count);
	}

	template <class Iter>
	FastMap(Iter first, Iter last)
		: m_C(2),
		m_SM_SCALING(1),
		m_num_pairs(0),
		m_num_operations(0)
	{
		// Could call fullRehash here instead to reuse code
		// Performed here now, otherwise a fullRehash would occur and do same operation after first insert
		m_capacity = (1 + m_C) * std::max(this->size(), size_t(4));	// Calculate new element count threshold
		auto bucket_count = s(m_capacity);							// Calculate new number of partitions/buckets in Top Level Table
		m_table.resize(bucket_count);								// Grow table if needed to accomodate new number of partitions/buckets
		m_hash = random_hash<K>(bucket_count);

		for (; first != last; ++first)
		{
			this->insert(std::move(*first));
		}
	}

	size_t size() const
	{
		return m_num_pairs;
	}

	// try to insert pair into the hash table
	bool insert(const pair_t& pair)
	{
		// check for duplicate key
		if (!count(pair.first)) return false;

		++m_num_operations;
		++m_num_pairs;

		// after a certain number of successful inserts, do a rebuild regardless
		if (m_num_operations > m_capacity) return rebuildAndInsert(pair);

		auto& bucket = getSubtable(pair.first);

		// create subtable if it doesn't exist
		if (!bucket) bucket = new FastLookupMap<K, V>();

		// if we can insert without growing the subtable, do that
		if (bucket->isUnderCapacity()) return bucket->insert(pair);

		// else we need to see what the effect of adding the pair to the subtable would be

		// Calculate new accumulated subtable allocation
		size_t total_bucket_count = bucket->bucketCountAfterInsert(); // sum of sj
		for (auto& b : m_table)
		{
			if (b && b != bucket) total_bucket_count += b->bucketCount();
		}

		if (isBelowCapacityThreshold(total_bucket_count)) return bucket->insert(pair);

		// insert would unbalance table, rebuild
		return rebuildAndInsert(pair);
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		++m_num_operations;

		if (count(key))
		{
			auto& usubtable = getSubtable(key);
			usubtable->erase(key);
			--m_num_pairs;
		}
		else
		{
			return false;
		}

		if (m_num_operations >= m_capacity)
		{
			fullRehash();
		}

		return true;
	}

	// return the value matching key
	V at(const K& key) const
	{
		if (!count(key)) throw std::out_of_range("FastMap::at");
		auto& usubtable = getSubtable(key);
		return !usubtable ? 0 : usubtable->at(key);
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		auto& bucket = getSubtable(key);
		return bucket && bucket->count(key);
	}

//private:

	// convenience functions for getting the unique_ptr for a key
	subtable_t*& getSubtable(const K& key)
	{
		return m_table.at(m_hash(key));
	}

	subtable_t* const& getSubtable(const K& key) const
	{
		return m_table.at(m_hash(key));
	}

	bool rebuildAndInsert(const pair_t& new_pair)
	{
		return fullRehash(new pair_t(new_pair));
	}

	bool fullRehash(pair_t* new_bucket_pair = nullptr)
	{
		// ensure duplicate keys are not added
		if (new_bucket_pair != nullptr && count(new_bucket_pair->first))
		{
			delete new_bucket_pair;
			new_bucket_pair = nullptr;
		}

		bucket_list_t bucket_pair_list;
		std::vector<size_t> hash_distribution;
		hashed_bucket_list_t hashed_bucket_list;

		size_t num_pairs = m_num_pairs;
		bucket_pair_list.reserve(num_pairs + 1);

		moveTableToList(m_table, bucket_pair_list);

		// add new pair, if specified
		if (new_bucket_pair)
		{
			bucket_pair_list.push_back(new_bucket_pair);
			++num_pairs;
		}

		m_capacity = (1 + m_C) * std::max(num_pairs, size_t(4)); // (M) Calculate new element number threshold
		auto bucket_count = s(m_capacity);                       // (s(M)) Calculate new number of partitions/buckets in Top Level Table
		m_table.resize(bucket_count);                            // Grow table if needed to accomodate new number of partitions/buckets

		m_hash = findBalancedHash(bucket_list, m_table.size(), hash_distribution);

		hashUpairList(bucket_list, hashed_bucket_list, m_hash, hash_distribution);

		// Reconstruct via hashed element list
		for (size_t i = 0; i < m_table.size(); ++i)
		{
			if (m_table[i] == nullptr)
			{
				if (hash_distribution[i] != 0)
				{
					m_table[i] = std::make_unique<FastLookupMap<K, V>>(hashed_bucket_list[i]);
				}
			}
			else
			{
				m_table[i]->rebuildTable(hashed_bucket_list[i]);
			}
		}

		m_num_operations = 0;
		return true;
	}

	void moveTableToList(table_t& table, bucket_list_t& bucket_list)
	{
		for (auto& bucket_subtable : table)
		{
			if (!bucket_subtable) continue;

			for (auto& bucket_pair : bucket_subtable->m_table)
			{
				if (bucket_pair)
				{
					bucket_list.push_back(bucket_pair);
					bucket_pair = nullptr;
				}
			}

			delete bucket_subtable;
			bucket_subtable = nullptr;
		}
	}

	hash_t findBalancedHash(const std::vector<bucket_t>& bucket_list, size_t num_buckets, std::vector<size_t>& hash_distribution)
	{
		hash_t hash;
		size_t total_bucket_count;
		hash_distribution.resize(num_buckets);

		do
		{
			// Reset variables declared outside loop
			total_bucket_count = 0;
			std::fill(hash_distribution.begin(), hash_distribution.end(), 0);

			hash = random_hash<K>(num_buckets);

			// Calculate hash distribution
			for (const auto& x : bucket_list)
			{
				++hash_distribution.at(hash(x->first));
			}

			// Determine number of buckets in resulting subtables
			for (size_t i = 0; i < num_buckets; ++i)
			{
				auto size = hash_distribution[i];						// (bj) count of elements in subtable
				auto capacity = 2 * size;								// (mj) capacity of subtable
				total_bucket_count += 2 * capacity * (capacity - 1);	// (sj sum) cumulative bucket count of subtables
			}

		}
		while (!isBelowCapacityThreshold(total_bucket_count));

		return hash;
	}

	void hashUpairList(std::vector<bucket_t>& bucket_list, hashed_bucket_list_t& hashed_bucket_list, hash_t hash, const std::vector<size_t>& hash_distribution)
	{
		hashed_bucket_list.resize(hash_distribution.size());

		// Reserve each hashed element list to its expected count
		for (size_t i = 0; i < hash_distribution.size(); ++i)
		{
			hashed_bucket_list[i].reserve(hash_distribution[i]);
		}

		// Distribute full element list over 2D hashed element list
		for (auto& e : bucket_list)
		{
			hashed_bucket_list.at(hash(e->first)).emplace_back(std::move(e));
		}
	}

	double calculateDPHThresh()
	{
		return ((32.0 * std::pow(m_capacity, 2)) / m_table.size()) + 4.0 * m_capacity;
	}

	bool isBelowCapacityThreshold(size_t bucket_count)
	{
		return (bucket_count - 4 * m_capacity) * m_table.size() <= 32 * m_capacity * m_capacity;
	}

	size_t s(size_t M) const
	{
		return m_SM_SCALING * M;
	}

	size_t hashKey(hash_t hash, const K& key) const
	{
		return hash(key);
	}

	size_t hashKey(hash_t hash, const pair_t& pair) const
	{
		return hashKey(hash, pair.first);
	}

	size_t hashKey(hash_t hash, const bucket_t& pair) const
	{
		return hashKey(hash, pair->first);
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

	table_t m_table;                // internal hash table
	hash_t m_hash;                  // hash function
	// TODO constants?
	size_t m_C;                     // Growth of M
	size_t m_SM_SCALING;            // Growth of Partitions/Buckets in Top Level Table
	// variables
	size_t m_capacity;              // (M) Threshold for total number of elements in Table
	size_t m_num_pairs;             // how many pairs are currently stored
	size_t m_num_operations;
};

#endif
