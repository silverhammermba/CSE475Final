#pragma once
#ifndef TEST_FAST_MAP_H
#define TEST_FAST_MAP_H

#include <vector>
#include "fast_map.h"
#include "gtest_include.h"

class AFastMap : public ::testing::Test
{
public:
	FastMap<int, int> m_map;
	std::pair<const int, int> m_pair;

	AFastMap()
		: m_pair {5, 6}
	{
	}
};

TEST_F(AFastMap, IsEmptyWhenCreated)
{
	EXPECT_EQ(0u, m_map.size());
	ASSERT_EQ(m_map.begin(), m_map.end());
}

TEST_F(AFastMap, CannotAccessMissingPair)
{
	EXPECT_EQ(0u, m_map.count(m_pair.first));
	EXPECT_THROW(m_map.at(m_pair.first), std::out_of_range);
}

TEST_F(AFastMap, CanInsertPair)
{
	ASSERT_EQ(true, m_map.insert(m_pair));
	EXPECT_EQ(1u, m_map.size());
	EXPECT_EQ(1u, m_map.count(m_pair.first));
	EXPECT_EQ(m_pair.second, m_map.at(m_pair.first));
}

TEST_F(AFastMap, CanErasePair)
{
	ASSERT_EQ(0u, m_map.erase(m_pair.first));

	m_map.insert(m_pair);

	EXPECT_EQ(1u, m_map.erase(m_pair.first));
	EXPECT_EQ(0u, m_map.count(m_pair.first));
	EXPECT_EQ(0u, m_map.size());
}

TEST_F(AFastMap, IsIterable)
{
	m_map.insert(m_pair);

	// first value should be what we just inserted
	auto it = m_map.begin();
	ASSERT_EQ(*it, m_pair);

	// we should be able to change it
	++(it->second);
	ASSERT_EQ(m_pair.second + 1, m_map.at(m_pair.first));

	// next value should be the end
	ASSERT_EQ(++it, m_map.end());
}

TEST_F(AFastMap, IsConstIterable)
{
	m_map.insert(m_pair);

	// first value should be what we just inserted
	auto it = m_map.cbegin();
	ASSERT_EQ(*it, m_pair);

	// next value should be the end
	ASSERT_EQ(++it, m_map.cend());
}

TEST_F(AFastMap, CanUseRangeBasedFor)
{
	m_map.insert(m_pair);

	for (auto& p : m_map)
	{
		ASSERT_EQ(p, m_pair);

		++p.second;
	}

	ASSERT_EQ(m_pair.second + 1, m_map.at(m_pair.first));

	for (const auto& p : m_map)
	{
		ASSERT_EQ(p.first, m_pair.first);
		ASSERT_EQ(p.second, m_pair.second + 1);
	}
}

TEST_F(AFastMap, CanInsertManyPairs)
{
	size_t count = 1000;

	std::vector<std::pair<int,int>> pairs;
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

TEST_F(AFastMap, CanBeRebuiltFully)
{
	int count = 1000;

	std::vector<std::pair<int, int>> pairs;
	for (int i = 0; i < count; ++i)
		pairs.push_back(std::make_pair(i, -i));

	for (const auto& pair : pairs) m_map.insert(pair);

	m_map.full_rebuild();

	EXPECT_EQ(count, m_map.size());
	
	for (const auto& pair : pairs)
	{
		EXPECT_EQ(1u, m_map.count(pair.first));
		EXPECT_EQ(pair.second, m_map.at(pair.first));
	}
}

#endif
