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

TEST_F(AFastMap, CanBeStaticallyConstructed)
{
	int count = 1000;

	std::vector<std::pair<int, int>> pairs;
	for (int i = 0; i < count; ++i)
		pairs.push_back(std::make_pair(i, -i));

	FastMap<int, int> map(pairs.begin(), pairs.end());

	EXPECT_EQ(size_t(pairs.size()), map.size());

	for (const auto& pair : pairs)
	{
		EXPECT_EQ(1u, map.count(pair.first));
		EXPECT_EQ(pair.second, map.at(pair.first));
	}
}

TEST_F(AFastMap, CanEraseManyPairs)
{
	size_t count = 1000;

	std::vector<std::pair<int, int>> pairs;
	for (size_t i = 0; i < count; ++i)
		pairs.push_back(std::make_pair(int(i), -int(i)));

	for (const auto& pair : pairs) m_map.insert(pair);

	for (const auto& pair : pairs) ASSERT_EQ(1u, m_map.erase(pair.first));

	// table rebuilds during inserts, so only tests elements after all inserts complete

	EXPECT_EQ(0, m_map.size());

	for (const auto& pair : pairs)
	{
		EXPECT_EQ(0u, m_map.count(pair.first));
		EXPECT_THROW(m_map.at(pair.first), std::out_of_range);
	}
}

TEST_F(AFastMap, CanBeRebuiltFully)
{
	int count = 1000;

	std::vector<std::pair<int, int>> pairs;
	for (int i = 0; i < count; ++i)
		pairs.push_back(std::make_pair(i, -i));

	for (const auto& pair : pairs) m_map.insert(pair);

	m_map.fullRehash();

	EXPECT_EQ(count, m_map.size());
	
	for (const auto& pair : pairs)
	{
		EXPECT_EQ(1u, m_map.count(pair.first));
		EXPECT_EQ(pair.second, m_map.at(pair.first));
	}
}

#endif
