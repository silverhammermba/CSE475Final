#pragma once
#ifndef TEST_FAST_LOOKUP_MAP_H
#define TEST_FAST_LOOKUP_MAP_H

#include <vector>
#include "fast_lookup_map.h"
#include "gtest_include.h"

class AFastLookupMap : public ::testing::Test
{
public:
	FastLookupMap<int, int> m_map;
	std::pair<const int, int> m_pair;

	AFastLookupMap()
		: m_pair {5, 6}
	{
	}
};

TEST_F(AFastLookupMap, IsEmptyWhenCreated)
{
	EXPECT_EQ(0u, m_map.size());
}

TEST_F(AFastLookupMap, CannotAccessMissingPair)
{
	EXPECT_EQ(0u, m_map.count(m_pair.first));
	EXPECT_THROW(m_map.at(m_pair.first), std::out_of_range);
}

TEST_F(AFastLookupMap, CanInsertPair)
{
	ASSERT_EQ(true, m_map.insert(m_pair));
	EXPECT_EQ(1u, m_map.size());
	EXPECT_EQ(1u, m_map.count(m_pair.first));
	EXPECT_EQ(m_pair.second, m_map.at(m_pair.first));
}

TEST_F(AFastLookupMap, CanErasePair)
{
	ASSERT_EQ(0u, m_map.erase(m_pair.first));

	m_map.insert(m_pair);

	EXPECT_EQ(1u, m_map.erase(m_pair.first));
	EXPECT_EQ(0u, m_map.count(m_pair.first));
	EXPECT_EQ(0u, m_map.size());
}

TEST_F(AFastLookupMap, CanInsertManyPairs)
{
	int count = 1000;

	std::vector<std::pair<int, int>> pairs;
	for (int i = 0; i < count; ++i)
		pairs.push_back(std::make_pair(i, -i));

	for (const auto& pair : pairs) ASSERT_EQ(true, m_map.insert(pair));

	// table rebuilds during inserts, so only test elements after all inserts complete

	EXPECT_EQ(size_t(count), m_map.size());

	for (const auto& pair : pairs)
	{
		EXPECT_EQ(1u, m_map.count(pair.first));
		EXPECT_EQ(pair.second, m_map.at(pair.first));
	}
}

#endif
