#ifndef FAST_MAP_H
#define FAST_MAP_H

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "fast_lookup_map.h"

// Starting number of buckets (s(M)?)
// s(M) -   "The number of sets into which the top level hash function is to partion the elements of S" (pg4)
//          "The parameter s(M) will be chosen to be Theta(n) so that the right hand side of this equation is O(n)" (pg5)
// Starting c value

template <class K, class V>
class FastMap
{
public:
	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<const K, V> pair_t;
	typedef std::unique_ptr<FastLookupMap<K, V>> ptr_t;
	typedef std::vector<ptr_t> table_t;

	typedef std::unique_ptr<pair_t> element_t;
	typedef std::vector<element_t> element_list_t;
	typedef std::vector<element_list_t> hashed_element_list_t;

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
		inline bool is_valid() const
		{
			// either outer_it as at the end or it is pointing to a nonempty FastLookupMap and inner_it is pointing to a pair
			return outer_it == outer_end || (*outer_it && inner_it != (*outer_it)->end());
		}

		// check if outer_it can be used to get a FastLookupMap
		inline bool outer_can_deref() const
		{
			return outer_it != outer_end && *outer_it;
		}

		// ensure we are in a valid state (by updating outer_it/inner_it)
		inline void make_valid()
		{
			while (!is_valid())
			{
				++outer_it; // go to the next bucket
				if (outer_can_deref()) inner_it = (*outer_it)->begin();
			}
		}

		// construct from hash table iterator
		explicit ForwardIterator(const o_itr_t& _outer_it, const o_itr_t& _outer_end)
			: outer_it{ _outer_it }, outer_end{ _outer_end }
		{
			if (outer_can_deref()) inner_it = (*outer_it)->begin();
			make_valid();
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
			make_valid();

			return *this;
		}

		// it++
		ForwardIterator operator++(int)
		{
			ForwardIterator prev{ outer_it, outer_end, inner_it };

			++inner_it;
			make_valid();

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

	FastMap(size_t num_buckets = 2)
		: m_num_pairs(0),
		m_c(1)
	{
		if (num_buckets == 0) throw std::out_of_range("FastMap num_buckets must not be zero");
		m_table.resize(num_buckets);
		m_hash = random_hash<K>(num_buckets);
	}

	size_t size() const
	{
		return m_num_pairs;
	}

	// try to insert pair into the hash table
	bool insert(const pair_t& pair)
	{
		auto& ptr = ptr_at(pair.first);
		if (!ptr) ptr_at(pair.first) = std::make_unique<FastLookupMap<K, V>>();
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

	// convenience functions for getting the unique_ptr for a key
	ptr_t& ptr_at(const K& key)
	{
		return m_table.at(m_hash(key));
	}

	const ptr_t& ptr_at(const K& key) const
	{
		return m_table.at(m_hash(key));
	}

	void full_rebuild()
	{
		full_rebuild(element_t());
	}
	void full_rebuild(const pair_t& new_pair)
	{
		full_rebuild(std::make_unique<pair_t>(new_pair));
	}
	void full_rebuild(element_t new_element)
	{
		element_list_t element_list;
		std::vector<size_t> hash_distribution;
		hashed_element_list_t hashed_element_list;

		move_table_to_list(m_table, element_list, m_num_pairs);

		// If arg new_element exists, push onto list and update count
		if (new_element != nullptr)
		{
			element_list.emplace_back(std::move(new_element));
			++m_num_pairs;
		}

		auto M = (1 + m_c) * std::max(m_num_pairs, size_t(4));

		m_hash = find_balanced_hash(element_list, m_table.size(), calculate_dph_thresh(m_table.size(), M), hash_distribution);

		hash_element_list(element_list, hashed_element_list, m_hash, hash_distribution);

		// Destruct Fast Lookup Map and reconstruct via hashed element list
		for (size_t i = 0; i < m_table.size(); ++i)
		{
			m_table[i].reset(new FastLookupMap<K, V>(hashed_element_list[i].begin(), hashed_element_list[i].end()));
		}
	}
	void move_table_to_list(table_t& table, element_list_t& element_list, size_t num_elements = 0)
	{
		// Can't use iterators for FastMap, as they dereference unique_ptr and we want to deep copy them

		if (element_list.size() != 0) element_list.reserve(num_elements);

		for (auto& bucket1 : table) // iterate over fast_map buckets
		{
			if (bucket1 != nullptr)
			{
				auto& slt = *bucket1.get();
				for (auto& u_pair : slt.m_table) // iterate over perfect_table buckets
				{
					if (u_pair != nullptr)
					{
						element_list.emplace_back(std::move(u_pair));
					}
				}

				/*auto& slt = *bucket1.get();
				for (auto it = slt.begin(); it != slt.end(); ++it)
				{
					element_list.emplace_back(std::move(*it));
				}*/
			}
		}
	}
	hash_t find_balanced_hash(const element_list_t& element_list, size_t num_buckets, double dph_thresh, std::vector<size_t>& hash_distribution)
	{
		hash_t hash;
		size_t sj_sum;
		hash_distribution.resize(num_buckets);

		do
		{
			// Reset variables declared outside loop
			sj_sum = 0;
			std::fill(hash_distribution.begin(), hash_distribution.end(), 0);

			hash = random_hash<K>(num_buckets);

			// Calculate hash distribution
			for (const auto& x : element_list)
			{
				++hash_distribution.at(hash(x->first));
			}

			// Determine size of resulting sub-tables
			for (size_t i = 0; i < num_buckets; ++i)
			{
				auto bj = hash_distribution[i]; // count of elements in sub-table
				auto mj = 2 * bj;               // capacity of sub-table
				sj_sum += 2 * mj * (mj - 1);    // cumulative size of sub-table
			}

		} while (sj_sum > dph_thresh);

		return hash;
	}
	void hash_element_list(element_list_t& element_list, hashed_element_list_t& hashed_element_list, hash_t hash, const std::vector<size_t>& hash_distribution)
	{
		hashed_element_list.resize(hash_distribution.size());

		// Resize each hashed element list to its expected count
		for (size_t i = 0; i < hash_distribution.size(); ++i)
		{
			hashed_element_list[i].reserve(hash_distribution[i]);
		}

		// Distribute full element list over 2D hashed element list
		for (auto& e : element_list)
		{
			hashed_element_list.at(hash(e->first)).emplace_back(std::move(e));
		}
	}
	double calculate_dph_thresh(size_t sM, size_t M)
	{
		return ((32.0 * std::pow(M, 2)) / sM) + 4.0 * M;
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
	size_t m_num_pairs;             // how many pairs are currently stored
	size_t m_c; // TODO figure out what this means
};

#endif
