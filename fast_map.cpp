
#include "fast_map.h"

template <class K>			using hash_t2 = std::function<size_t(K)>;
template <class K, class V> using pair_t2 = std::pair<K, V>;
template <class K, class V> using element_t2 = std::unique_ptr<pair_t2<K, V>>;
template <class K, class V> using element_list_t2 = std::vector<element_t2<K, V>>;
template <class K, class V> using hashed_element_list_t2 = std::vector<element_list_t2<K, V>>;
template <class K, class V> using ptr_t2 = std::unique_ptr<FastLookupMap<K, V>>;
template <class K, class V> using table_t2 = std::vector<ptr_t2<K, V>>;

template <class K, class V>
hash_t2<K> full_rehash(table_t2<K, V>& table, size_t num_elements, size_t c)
{
	return full_rehash(table, num_elements, c, nullptr);
}
template <class K, class V>
hash_t2<K> full_rehash(table_t2<K, V>& table, size_t num_elements, size_t c, const pair_t2<K, V>& new_pair)
{
	return full_rehash(table, num_elements, c, std::make_unique<pair_t2<K, V>>(new_pair));
}
template <class K, class V>
hash_t2<K> full_rehash(table_t2<K, V>& table, size_t num_elements, size_t c, element_t2<K, V>& new_element)
{
	element_list_t2<K, V> element_list;
	std::vector<size_t> hash_distribution;
	hashed_element_list_t2<K, V> hashed_element_list;

	auto num_buckets = table.size();

	move_table_to_list(table, element_list, num_elements);

	// If arg new_element exists, push onto list and update count
	if (new_element != nullptr)
	{
		element_list.emplace_back(new_element);
		++num_elements;
	}

	auto M = (1 + c) * std::max(num_elements, 4);

	auto hash = find_balanced_hash(element_list, num_buckets, M, hash_distribution);

	hash_element_list(element_list, hashed_element_list, hash, hash_distribution);

	// Destruct Perfect Tables and reconstruct via hashed element list
	for (size_t i = 0; i < table.size(); ++i)
	{
		table[i].reset(new FastLookupMap(hashed_element_list[i]));
	}

	return hash;
}
template <class K, class V>
void move_table_to_list(table_t2<K, V>& table, element_list_t2<K, V>& list, size_t num_elements = 0)
{
	if (element_list != 0) element_list.reserve(num_elements);

	for (;;;) // iterate over fast_map buckets
		{
			if (;;;) // iterate over perfect_table buckets
			{
				//list.emplace_back(std::move(m_table));
			}
		}
}
template <class K, class V>
hash_t2<K> find_balanced_hash(const element_list_t2<K, V>& element_list, size_t sM, size_t M, std::vector<size_t>& hash_distribution)
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
template <class K, class V>
void hash_element_list(element_list_t2<K, V>& element_list, hashed_element_list_t2<K, V>& hashed_element_list, hash_t2<K> hash, std::vector<size_t> hash_distribution)
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

/*

//template <class K, class V> hash_t full_rehash(table_t& table, size_t num_elements, size_t c);
//template <class K, class V> hash_t full_rehash(table_t& table, size_t num_elements, size_t c, pair_t& new_element);
//template <class K, class V> void move_table_to_list(table_t& table, element_list_t& list, size_t num_elements = 0);
//template <class K, class V> hash_t find_balanced_hash(const element_list_t& element_list, size_t sM, size_t M, std::vector<size_t>& hash_distribution);
//template <class K, class V> void hash_element_list(element_list_t& element_list, hashed_element_list_t& hashed_element_list, hash_t hash, std::vector<size_t> hash_distribution);

template <class K, class V>
FastMap<K, V>::FastMap()
{
}

//template <class K, class V>
//FastMap<K, V>::FastMap(size_t num_buckets = 2)
//	:m_num_pairs{ 0 },
//	m_c(1)
//{
//	if (num_buckets == 0) throw std::out_of_range("FastMap num_buckets must not be zero");
//	m_table.resize(num_buckets);
//	m_hash = random_hash<K>(unsigned int(num_buckets));
//}
template <class K, class V>
size_t FastMap<K, V>::size() const
{
	return m_num_pairs;
}
template <class K, class V>
bool FastMap<K, V>::insert(const pair_t& pair)
{
	auto& ptr = ptrAt(pair.first);
	if (!ptr) ptrAt(pair.first) = std::make_unique<FastLookupMap<K, V>>();
	auto ret = ptr->insert(pair);
	if (ret) ++m_num_pairs;
	return ret;
}
template <class K, class V>
size_t FastMap<K, V>::erase(const K& key)
{
	auto& ptr = ptrAt(key);
	auto ret = !ptr ? 0 : ptr->erase(key);
	if (ret) --m_num_pairs;
	return ret;
}
template <class K, class V>
V FastMap<K, V>::at(const K& key) const
{
	if (!count(key)) throw std::out_of_range("FastMap::at");
	auto& ptr = ptrAt(key);
	return !ptr ? 0 : ptr->at(key);
}
template <class K, class V>
size_t FastMap<K, V>::count(const K& key) const
{
	auto& ptr = ptrAt(key);
	return !ptr ? 0 : ptr->count(key);
}
//template <class K, class V>
//ptr_t& FastMap<K, V>::ptrAt(const K& key)
//{
//	return m_table.at(m_hash(key));
//}
//template <class K, class V>
//const FastMap<K, V>::ptr_t& ptrAt(const K& key) const
//{
//	return m_table.at(m_hash(key));
//}

// Free (non-friend non-member) functions

//template <class K, class V>
//hash_t full_rehash(table_t& table, size_t num_elements, size_t c)
//{
//	return full_rehash(table, num_elements, c, nullptr);
//}
//template <class K, class V>
//hash_t full_rehash(table_t& table, size_t num_elements, size_t c, pair_t& new_element)
//{
//	element_list_t element_list;
//	std::vector<size_t> hash_distribution;
//	hashed_element_list_t hashed_element_list;
//
//	auto num_buckets = table.size();
//
//	move_table_to_list(table, element_list, num_elements);
//
//	// If arg new_element exists, push onto list and update count
//	if (new_element != nullptr)
//	{
//		element_list.emplace_back(new_element);
//		++num_elements;
//	}
//
//	auto M = (1 + c) * std::max(num_elements, 4);
//
//	auto hash = find_balanced_hash(element_list, num_buckets, M, hash_distribution);
//
//	hash_element_list(element_list, hashed_element_list, hash, hash_distribution);
//
//	// Destruct Perfect Tables and reconstruct via hashed element list
//	for (size_t i = 0; i < table.size(); ++i)
//	{
//		table[i].reset(new FastLookupMap(hashed_element_list[i]));
//	}
//
//	return hash;
//}
//template <class K, class V>
//void move_table_to_list(table_t& table, element_list_t& list, size_t num_elements = 0)
//{
//	if (element_list != 0) element_list.reserve(num_elements);
//
//	for (;;;) // iterate over fast_map buckets
//	{
//		if (;;;)
//		{
//			//list.emplace_back(std::move(m_table));
//		}
//	}
//}
//template <class K, class V>
//hash_t find_balanced_hash(const element_list_t& element_list, size_t sM, size_t M, std::vector<size_t>& hash_distribution)
//{
//	size_t sj_sum;
//	hash_distribution.resize(sM);
//
//	do
//	{
//		// Reset variables declared outside loop
//		sj_sum = 0;
//		std::fill(hash_distribution.begin(), hash_distribution.end(), 0);
//
//		hash = random_hash(sM);
//
//		// Calculate hash distribution
//		for (const auto& x : element_list)
//		{
//			++hash_distribution.at(hash(x->first));
//		}
//
//		// Determine size of resulting sub-tables
//		for (size_t i = 0; i < sM; ++i)
//		{
//			auto bj = hash_distribution[i];		// count of elements in sub-table
//			auto mj = 2 * bj;					// capacity of sub-table
//			sj_sum += 2 * mj * (mj - 1);		// size of sub-table
//		}
//
//	} while (sj_sum > 32 * M ^ 2 / (sM + 4 * M));
//}
//template <class K, class V>
//void hash_element_list(element_list_t& element_list, hashed_element_list_t& hashed_element_list, hash_t hash, std::vector<size_t> hash_distribution)
//{
//	hashed_element_list.resize(hash_distribution.size());
//
//	// Resize each hashed element list to its expected count
//	for (size_t i = 0; i < hash_distribution.size(); ++i)
//	{
//		hashed_element_list[i].resize(hash_distribution[i]);
//	}
//
//	// Distribute full element list over 2D hashed element list
//	for (auto& e : element_list)
//	{
//		hashed_element_list.at(hash(e->first)).emplace_back(std::move(e));
//	}
//}

*/