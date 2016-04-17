#ifndef TEST_FAST_LOOKUP_MAP_TIMING_H
#define TEST_FAST_LOOKUP_MAP_TIMING_H

#include <atomic>
#include <chrono>
#include <thread>
#include "fast_lookup_map.h"

void timeFLMReadConcurrently(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::atomic<int> barrier{ 0 };
	std::chrono::high_resolution_clock::time_point start_time, end_time;
	
	FastLookupMap<int, int> map(pairs.begin(), pairs.end());

	auto read = [&](int id)
	{
		++barrier;
		while (barrier < num_threads);
		if (id == 0) start_time = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < count; ++i) map.at(i);

		--barrier;
		while (barrier > 0);
		if (id == num_threads - 1) end_time = std::chrono::high_resolution_clock::now();
	};

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
		threads.emplace_back(std::bind(read, i));
	for (int i = 0; i < num_threads; ++i)
		threads[i].join();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

void timeFLMReadsSerially(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::atomic<int> barrier{ 0 };
	std::atomic<int> serial_barrier{ 0 };
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastLookupMap<int, int> map(pairs.begin(), pairs.end());

	auto read = [&](int id)
	{
		++barrier;
		while (barrier < num_threads);
		if (id == 0) start_time = std::chrono::high_resolution_clock::now();

		while (serial_barrier < id); // wait your turn

		for (int i = 0; i < count; ++i) map.at(i);

		++serial_barrier;

		if (id == num_threads - 1) end_time = std::chrono::high_resolution_clock::now();
	};

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
		threads.emplace_back(std::bind(read, i));
	for (int i = 0; i < num_threads; ++i)
		threads[i].join();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

void timeFLMReadsSeriallyAgain(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastLookupMap<int, int> map(pairs.begin(), pairs.end());

	start_time = std::chrono::high_resolution_clock::now();
	for (int j = 0; j < num_threads; ++j)
	{
		for (int i = 0; i < count; ++i) map.at(i);
	}
	end_time = std::chrono::high_resolution_clock::now();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

void timeFLMInsertsConcurrentlyly(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::atomic<int> barrier{ 0 };
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastLookupMap<int, int> map;

	auto read = [&](int id)
	{
		++barrier;
		while (barrier < num_threads);
		if (id == 0) start_time = std::chrono::high_resolution_clock::now();

		int check = 0;
		for (int i = 0; i < count; ++i)
		{
			if (i % num_threads == id)
			{
				if (map.insert(pairs[i]) != 1)
				{
					throw std::exception();
				}
			}
		}

		--barrier;
		while (barrier > 0);
		if (id == num_threads - 1) end_time = std::chrono::high_resolution_clock::now();

		return check;
	};

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
		threads.emplace_back(std::bind(read, i));
	for (int i = 0; i < num_threads; ++i)
		threads[i].join();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

void timeFLMInsertsSerially(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::atomic<int> barrier{ 0 };
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastLookupMap<int, int> map;

	auto read = [&](int id)
	{
		if (id == 0) start_time = std::chrono::high_resolution_clock::now();

		while (barrier < id);

		int check = 0;
		for (int i = 0; i < count; ++i)
		{
			if (i % num_threads == id)
			{
				if (map.insert(pairs[i]) != 1)
				{
					throw std::exception();
				}
			}
		}

		++barrier;

		if (id == num_threads - 1) end_time = std::chrono::high_resolution_clock::now();

		return check;
	};

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; ++i)
	{
		threads.emplace_back(std::bind(read, i));
		threads[i].join();
	}

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

void timeFLMInsertsSeriallyAgain(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastLookupMap<int, int> map;

	start_time = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < count; ++i)
	{
		if (map.insert(pairs[i]) != 1)
		{
			throw std::exception();
		}
	}
	end_time = std::chrono::high_resolution_clock::now();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

void runFastLookupMapTiming()
{
	int num_threads = 4;
	int count = 4000;
	std::vector<std::pair<const int, int>> pairs;
	for (int i = 0; i < count; ++i) pairs.emplace_back(i, -i);
	
	timeFLMReadConcurrently(num_threads, pairs);
	timeFLMReadsSerially(num_threads, pairs);
	timeFLMReadsSeriallyAgain(num_threads, pairs);
	timeFLMInsertsConcurrentlyly(num_threads, pairs);
	timeFLMInsertsSerially(num_threads, pairs);
	timeFLMInsertsSeriallyAgain(num_threads, pairs);
}

#endif