#ifndef TEST_FAST_LOOKUP_MAP_H
#define TEST_FAST_LOOKUP_MAP_H

#include <stdexcept>
#include <utility>
#include <vector>

#include "gtest_include.h"
#include "fast_lookup_map.h"

class AFastLookupMap : public ::testing::Test
{
public:
	FastLookupMap<int, int> m_map;
};

TEST_F(AFastLookupMap, IsEmptyWhenCreated)
{
	EXPECT_EQ(0u, m_map.size());
}

TEST_F(AFastLookupMap, CannotAccessMissingPair)
{
	int k{ 5 };
	EXPECT_EQ(0u, m_map.count(k));
	EXPECT_THROW(m_map.at(k), std::out_of_range);
}

TEST_F(AFastLookupMap, CanInsertPair)
{
	int k{ 5 };
	int v{ 6 };
	ASSERT_EQ(true, m_map.insert(std::make_pair(k, v)));
	EXPECT_EQ(1u, m_map.size());
	EXPECT_EQ(1u, m_map.count(k));
	EXPECT_EQ(v, m_map.at(k));
}

TEST_F(AFastLookupMap, CanErasePair)
{
	int k{ 5 };
	ASSERT_EQ(0u, m_map.erase(k));

	m_map.insert(std::make_pair(k, k + 1));

	EXPECT_EQ(1u, m_map.erase(k));
	EXPECT_EQ(0u, m_map.count(k));
	EXPECT_EQ(0u, m_map.size());
}

TEST_F(AFastLookupMap, CanInsertManyPairs)
{
	size_t count = 1000;

	std::vector<std::pair<int, int>> pairs;
	for (size_t i = 0; i < count; ++i)
		pairs.push_back(std::make_pair(int(i), -int(i)));

	for (const auto& pair : pairs) ASSERT_EQ(true, m_map.insert(pair));

	// table rebuilds during inserts, so only tests elements after all inserts complete

	EXPECT_EQ(count, m_map.size());

	for (const auto& pair : pairs)
	{
		EXPECT_EQ(1u, m_map.count(pair.first));
		EXPECT_EQ(pair.second, m_map.at(pair.first));
	}
}

#endif
