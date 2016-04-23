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
	typedef std::unique_ptr<pair_t> upair_t;
	typedef std::vector<upair_t> upair_list_t;
	typedef std::vector<upair_list_t> hashed_upair_list_t;
	typedef std::unique_ptr<FastLookupMap<K, V>> subtable_t;
	typedef std::vector<subtable_t> table_t;

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
		m_prime(2),
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
		m_prime(2),
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
		++m_num_operations;
		if (m_num_operations > m_capacity)
		{
			fullRehash(pair);
		}
		else
		{
			auto bucket = hashKey(m_hash, pair);
			auto& usubtable = getSubtable(pair.first);

			// If subtable doesn't exist create it, insert the element, and return
			if (usubtable == nullptr)
			{
				usubtable = std::make_unique<FastLookupMap<K, V>>();
				usubtable->insert(pair);
				++m_num_pairs;
				return true;
			}

			// If the key exists in the subtable return
			if (usubtable->count(pair.first))
			{
				return false;
			}

			if (usubtable->size() + 1 <= usubtable->capacity())
			{
				if (!usubtable->isCollision(pair.first))
				{
					usubtable->insert(pair);
				}
				else
				{
					usubtable->rebuildTable(usubtable->bucketCount(), std::make_unique<pair_t>(pair));
				}
			}
			else
			{
				auto capacity = usubtable->capacity();									// (mj)
				auto new_capacity = 2 * std::max<size_t>(1, capacity);					// (mj)
				auto new_subtable_bucket_count = 2 * new_capacity * (new_capacity - 1);	// (sj)

				// Calculate new accumulated subtable allocation
				size_t total_bucket_count = 0;											// (sum of sj)
				for (size_t i = 0; i < m_table.size(); ++i)
				{
					if (i == bucket)
					{
						total_bucket_count += new_subtable_bucket_count;
					}
					else if (m_table[i] != nullptr)
					{
						total_bucket_count += m_table[i]->bucketCount();
					}
				}

				auto dph_thresh = calculateDPHThresh();
				if (total_bucket_count <= dph_thresh)
				{
					usubtable->rebuildTable(new_subtable_bucket_count, std::make_unique<pair_t>(pair));
				}
				else
				{
					fullRehash(pair);
				}
			}
		}

		++m_num_pairs;
		return true;
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
		auto& usubtable = getSubtable(key);
		return !usubtable ? 0 : usubtable->count(key);
	}

//private:

	// convenience functions for getting the unique_ptr for a key
	subtable_t& getSubtable(const K& key)
	{
		return m_table.at(m_hash(key));
	}

	const subtable_t& getSubtable(const K& key) const
	{
		return m_table.at(m_hash(key));
	}

	void fullRehash(const pair_t& new_pair)
	{
		fullRehash(std::make_unique<pair_t>(new_pair));
	}

	void fullRehash(upair_t new_upair = upair_t())
	{
		// We do not want duplicate keys added to our table, otherwise our calculateHash/isHashPerfect fcns will loop indefinitely
		if (new_upair != nullptr && count(new_upair->first))
		{
			new_upair.reset();
		}

		upair_list_t upair_list;
		std::vector<size_t> hash_distribution;
		hashed_upair_list_t hashed_upair_list;

		size_t num_pairs = this->size();
		upair_list.reserve(num_pairs + 1);

		moveTableToList(m_table, upair_list);

		// If arg new_upair exists, push onto list and update count
		if (new_upair != nullptr)
		{
			upair_list.emplace_back(std::move(new_upair));
			++num_pairs;
		}

		m_capacity = (1 + m_C) * std::max(num_pairs, size_t(4));	// (M) Calculate new element number threshold
		auto bucket_count = s(m_capacity);							// (s(M)) Calculate new number of partitions/buckets in Top Level Table
		m_table.resize(bucket_count);								// Grow table if needed to accomodate new number of partitions/buckets

		m_hash = findBalancedHash(upair_list, m_table.size(), calculateDPHThresh(), hash_distribution);

		hashUpairList(upair_list, hashed_upair_list, m_hash, hash_distribution);

		// Reconstruct via hashed element list
		for (size_t i = 0; i < m_table.size(); ++i)
		{
			if (m_table[i] == nullptr)
			{
				if (hash_distribution[i] != 0)
				{
					m_table[i] = std::make_unique<FastLookupMap<K, V>>(hashed_upair_list[i].begin(), hashed_upair_list[i].end());
				}
			}
			else
			{
				m_table[i]->rebuildTable(hashed_upair_list[i].begin(), hashed_upair_list[i].end());
			}
		}

		m_num_operations = 0;
	}

	void moveTableToList(table_t& table, upair_list_t& upair_list)
	{
		// Can't use iterators, as they dereference unique_ptr and we want to deep copy them

		for (auto& usubtable : table) // iterate over FastMap buckets
		{
			if (usubtable != nullptr)
			{
				auto& subtable = *usubtable.get();
				for (auto& upair : subtable.m_table) // iterate over perfect_table buckets
				{
					if (upair != nullptr)
					{
						upair_list.emplace_back(std::move(upair));
					}
				}
			}
		}
	}

	hash_t findBalancedHash(const upair_list_t& upair_list, size_t num_buckets, double dph_thresh, std::vector<size_t>& hash_distribution)
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
			for (const auto& x : upair_list)
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

		} while (total_bucket_count > dph_thresh);

		return hash;
	}

	void hashUpairList(upair_list_t& upair_list, hashed_upair_list_t& hashed_upair_list, hash_t hash, const std::vector<size_t>& hash_distribution)
	{
		hashed_upair_list.resize(hash_distribution.size());

		// Reserve each hashed element list to its expected count
		for (size_t i = 0; i < hash_distribution.size(); ++i)
		{
			hashed_upair_list[i].reserve(hash_distribution[i]);
		}

		// Distribute full element list over 2D hashed element list
		for (auto& e : upair_list)
		{
			hashed_upair_list.at(hash(e->first)).emplace_back(std::move(e));
		}
	}

	double calculateDPHThresh()
	{
		return ((32.0 * std::pow(m_capacity, 2)) / m_table.size()) + 4.0 * m_capacity;
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

	size_t hashKey(hash_t hash, const upair_t& pair) const
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
	unsigned int m_prime;           // prime number used by hash function
	size_t m_capacity;              // (M) Threshold for total number of elements in Table
	size_t m_num_pairs;             // how many pairs are currently stored
	size_t m_num_operations;
};

#endif
