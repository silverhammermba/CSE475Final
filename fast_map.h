
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
		auto& ptr = ptrAt(pair.first);
		if (!ptr) ptrAt(pair.first) = std::make_unique<PerfectTable<K,V>>();
		auto ret = ptr->insert(pair);
		if (ret) ++m_num_pairs;
		return ret;
	}

	// remove pair matching key from the table
	size_t erase(const K& key)
	{
		auto& ptr = ptrAt(key);
		auto ret = !ptr ? 0 : ptr->erase(key);
		if (ret) --m_num_pairs;
		return ret;
	}

	// return the value matching key
	V at(const K& key) const
	{
		if (!count(key)) throw std::out_of_range("FastMap::at");
		auto& ptr = ptrAt(key);
		return !ptr ? 0 : ptr->at(key);
	}

	// return 1 if pair matching key is in table, else return 0
	size_t count(const K& key) const
	{
		auto& ptr = ptrAt(key);
		return !ptr ? 0 : ptr->count(key);
	}

private:
	// convenience functions for getting the unique_ptr for a key
	inline ptr_t& ptrAt(const K& key)
	{
		return m_table.at(m_hash(key));
	}

	inline const ptr_t& ptrAt(const K& key) const
	{
		return m_table.at(m_hash(key));
	}
	
	void fullRehash(table_t& table, size_t num_elements, size_t c)
	{
		fullRehash(table, num_elements, c, nullptr);
	}
	hash_t fullRehash(table_t& table, size_t num_elements, size_t c, pair_t& new_element)
	{
		element_list_t element_list;
		std::vector<size_t> hash_distribution;
		hashed_element_list_t hashed_element_list;

		auto num_buckets = table.size();

		moveTableToList(table, element_list, num_elements);

		// If arg new_element exists, push onto list and update count
		if (new_element != nullptr)
		{
			element_list.emplace_back(new_element);
			++num_elements;
		}

		auto M = (1 + c) * std::max(num_elements, 4);
				
		auto hash = findBalancedHash(element_list, num_buckets, M, hash_distribution);

		hashElementList(element_list, hashed_element_list, hash, hash_distribution);

		// Destruct Perfect Tables and reconstruct via hashed element list
		for (size_t i = 0; i < table.size(); ++i)
		{
			table[i].reset(new PerfectTable(hashed_element_list[i]));
		}

		return hash;
	}
	void moveTableToList(table_t& table, element_list_t& list, size_t num_elements = 0)
	{
		if (element_list != 0) element_list.reserve(num_elements);

		for (;;;/*iterate over fast_map buckets*/)
		{
			if (;;;/*iterate over perfect_table buckets*/)
			{
				//list.emplace_back(std::move(m_table));
			}
		}
	}
	hash_t findBalancedHash(const element_list_t& element_list, size_t sM, size_t M, std::vector<size_t>& hash_distribution)
	{
		size_t sj_sum;
		hash_distribution.resize(sM);

		do
		{
			// Reset variables declared outside loop
			sj_sum = 0;
			std::fill(hash_distribution.begin(), hash_distribution.end(), 0);
			
			hash = random_hash(sM);

			// Calculate hash distribution
			for (const auto& x : element_list)
			{
				++hash_distribution.at(hash(x->first));
			}

			// Determine size of resulting sub-tables
			for (size_t i = 0; i < sM; ++i)
			{
				auto bj = hash_distribution[i];		// count of elements in sub-table
				auto mj = 2 * bj;					// capacity of sub-table
				sj_sum += 2 * mj * (mj - 1);		// size of sub-table
			}

		} while (sj_sum > 32 * M ^ 2 / (sM + 4 * M));
	}
	void hashElementList(element_list_t& element_list, hashed_element_list_t& hashed_element_list, hash_t hash, std::vector<size_t> hash_distribution)
	{
		hashed_element_list.resize(hash_distribution.size());

		// Resize each hashed element list to its expected count
		for (size_t i = 0; i < hash_distribution.size(); ++i)
		{
			hashed_element_list[i].resize(hash_distribution[i]);
		}

		// Distribute full element list over 2D hashed element list
		for (auto& e : element_list)
		{
			hashed_element_list.at(hash(e->first)).emplace_back(std::move(e));
		}
	}
	
	table_t m_table;                // internal hash table
	hash_t m_hash;                  // hash function
	size_t m_num_pairs;				// how many pairs are currently stored
	size_t m_M;
	size_t m_c;
};