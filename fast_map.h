
#include "gtest_include.h"
#include "perfect_table.h"

// Starting number of buckets (s(M)?)
// s(M) -   "The number of sets into which the top level hash function is to partion the elements of S" (pg4)
//			"The parameter s(M) will be chosen to be Odash(n) so that the right hand side of this equation is O(n)" (pg5)
// Starting c value

template <class K, class V>
class FastMap
{
	typedef std::function<size_t(K)> hash_t;
	typedef std::pair<K, V> pair_t;
	typedef std::unique_ptr<PerfectTable<K, V>> ptr_t;
	typedef std::vector<ptr_t> table_t;

	typedef std::unique_ptr<pair_t> element_t;
	typedef std::vector<element_t> element_list_t;
	typedef std::vector<element_list_t> hashed_element_list_t;
public:
	FastMap(size_t num_buckets = 2)
		:m_num_pairs{ 0 },
		m_c(1)
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

	void move_table_to_list(table_t& table, element_list_t& list)
	{
		for (;;;/*iterate over fast_map buckets*/)
		{
			if (;;;/*iterate over perfect_table buckets*/)
			{
				//list.emplace_back(std::move(m_table));
			}
		}
	}

	void full_rehash(/* X */)
	{
		element_list_t element_list;
		element_list.reserve(m_num_pairs + 1);
		move_table_to_list(m_table, element_list);
		// push back X

		auto element_count = element_list.size();
		m_M = (1 + m_c) * std::max(element_count, 4);
				
		// Find balanced hash
		hash_t hash;
		auto sM = m_table.size();
		auto M = m_M;
		std::vector<size_t> hash_distribution(sM);
		{
			size_t sj_sum;
			std::vector<size_t> sj(sM);
			do
			{
				std::fill(hash_distribution.begin(), hash_distribution.end(), 0);
				hash = random_hash(sM);

				// Find hash distribution
				for (const auto& x : element_list)
				{
					++hash_distribution.at(hash(x->first));
				}

				// Determine size of resulting sub-tables
				for (size_t i = 0; i < sM; ++i)
				{
					auto bj = hash_distribution[i];		// count of elements in Perfect Table
					auto mj = 2 * bj;					// capacity of Perfect Table
					sj[i] = 2 * mj * (mj - 1);			// size of Perfect Table
				}
				sj_sum = std::accumulate(sj.cbegin(), sj.cend(), 0);

			} while (sj_sum > 32 * M ^ 2 / (sM + 4 * M));
		}

		m_hash = hash;

		// Build list of lists
		// Destruct and rebuild FSK Perfect Table

		// Partition element list into list of hashed element list
		std::vector<element_list_t> hashed_element_list(m_table.size());
		// args: hashed_element_list, hash_distribution, element_list, hash
		{
			// Resize each hashed element list to its expected count
			for (size_t i = 0; i < hash_distribution.size(); ++i)
			{
				hashed_element_list[i].resize(hash_distribution[i]);
			}

			// Distribute full list over 2D hashed element list
			for (auto& e : element_list)
			{
				hashed_element_list.at(m_hash(e->first)).emplace_back(std::move(e));
			}
		}

		// Destruct Perfect Tables and recreate via FSK static method
		for (size_t i = 0; i < m_table.size(); ++i)
		{
			m_table[i].reset(new PerfectTable(hashed_element_list[i]));
		}

	}
	
	table_t m_table;                // internal hash table
	hash_t m_hash;                  // hash function
	size_t m_num_pairs;				// how many pairs are currently stored
	size_t m_M;
	size_t m_c;
};