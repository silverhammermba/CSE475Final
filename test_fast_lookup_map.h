#pragma once
#ifndef TEST_FAST_LOOKUP_MAP_H
#define TEST_FAST_LOOKUP_MAP_H

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include "fast_lookup_map.h"
#include "gtest_include.h"

class AFastLookupMap : public ::testing::Test
{
public:
	FastLookupMap<int, int> m_map;
	std::pair<const int, int> m_pair;
	std::vector<std::pair<const int, int>> m_pairs;

	AFastLookupMap()
		: m_pair {5, 6}
	{
		int count = 4000;
		for (int i = 0; i < count; ++i) m_pairs.emplace_back(i, -i);
	}
};

TEST_F(AFastLookupMap, IsEmptyWhenCreated)
{
	EXPECT_EQ(0u, m_map.size());
	ASSERT_EQ(m_map.begin(), m_map.end());
	ASSERT_EQ(m_map.cbegin(), m_map.cend());
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

TEST_F(AFastLookupMap, IsIterable)
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

TEST_F(AFastLookupMap, IsConstIterable)
{
	m_map.insert(m_pair);

	// first value should be what we just inserted
	auto it = m_map.cbegin();
	ASSERT_EQ(*it, m_pair);

	// next value should be the end
	ASSERT_EQ(++it, m_map.cend());
}

TEST_F(AFastLookupMap, CanUseRangeBasedFor)
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

TEST_F(AFastLookupMap, CanBeStaticallyConstructed)
{
	int count = 1000;

	std::vector<std::pair<int, int>> pairs;
	for (int i = 0; i < count; ++i)
		pairs.push_back(std::make_pair(i, -i));

	FastLookupMap<int, int> map(pairs.begin(), pairs.end());
	
	EXPECT_EQ(size_t(pairs.size()), map.size());

	for (const auto& pair : pairs)
	{
		EXPECT_EQ(1u, map.count(pair.first));
		EXPECT_EQ(pair.second, map.at(pair.first));
	}
}

TEST_F(AFastLookupMap, CanBeReadConcurrently)
{
	int num_threads = 4;
	int count = m_pairs.size();
	std::atomic<int> barrier {0};
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastLookupMap<int, int> map(m_pairs.begin(), m_pairs.end());


	auto read = [&](int id)
	{
		++barrier;
		while (barrier < num_threads);
		if (id == 0)
			start_time = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < count; ++i) map.at(i);

		--barrier;
		while (barrier > 0);
		if (id == num_threads - 1)
			end_time = std::chrono::high_resolution_clock::now();
	};

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
		threads.emplace_back(std::bind(read, i));
	for (int i = 0; i < num_threads; ++i)
		threads[i].join();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

TEST_F(AFastLookupMap, CanBeReadSerially)
{
	int num_threads = 4;
	int count = m_pairs.size();
	std::atomic<int> barrier {0};
	std::atomic<int> serial_barrier {0};
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastLookupMap<int, int> map(m_pairs.begin(), m_pairs.end());


	auto read = [&](int id)
	{
		++barrier;
		while (barrier < num_threads);
		if (id == 0)
			start_time = std::chrono::high_resolution_clock::now();

		while (serial_barrier < id); // wait your turn

		for (int i = 0; i < count; ++i) map.at(i);

		++serial_barrier;

		if (id == num_threads - 1)
			end_time = std::chrono::high_resolution_clock::now();
	};

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
		threads.emplace_back(std::bind(read, i));
	for (int i = 0; i < num_threads; ++i)
		threads[i].join();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

TEST_F(AFastLookupMap, CanBeReadALot)
{
	int num_threads = 4;
	int count = m_pairs.size();
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastLookupMap<int, int> map(m_pairs.begin(), m_pairs.end());


	start_time = std::chrono::high_resolution_clock::now();
	for (int j = 0; j < num_threads; ++j)
	{
		for (int i = 0; i < count; ++i) map.at(i);
	}
	end_time = std::chrono::high_resolution_clock::now();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

#endif
