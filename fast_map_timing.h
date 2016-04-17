#ifndef TEST_FAST_LOOKUP_MAP_TIMING_H
#define TEST_FAST_LOOKUP_MAP_TIMING_H

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include "fast_map.h"

void timeReadConcurrently(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::atomic<int> barrier{ 0 };
	std::chrono::high_resolution_clock::time_point start_time, end_time;
	
	FastMap<int, int> map(pairs.begin(), pairs.end());

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

void timeReadsSerially(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::atomic<int> barrier{ 0 };
	std::atomic<int> serial_barrier{ 0 };
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastMap<int, int> map(pairs.begin(), pairs.end());

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

void timeReadsSeriallyAgain(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastMap<int, int> map(pairs.begin(), pairs.end());

	start_time = std::chrono::high_resolution_clock::now();
	for (int j = 0; j < num_threads; ++j)
	{
		for (int i = 0; i < count; ++i) map.at(i);
	}
	end_time = std::chrono::high_resolution_clock::now();

	std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
}

void timeInsertsConcurrentlyly(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::atomic<int> barrier{ 0 };
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastMap<int, int> map;

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

void timeInsertsSerially(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::atomic<int> barrier{ 0 };
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastMap<int, int> map;

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

void timeInsertsSeriallyAgain(size_t num_threads, const std::vector<std::pair<const int, int>>& pairs)
{
	auto count = pairs.size();
	std::chrono::high_resolution_clock::time_point start_time, end_time;

	FastMap<int, int> map;

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

void runFastMapTiming()
{
	int num_threads = 4;
	int count = 4000;
	std::vector<std::pair<const int, int>> pairs;
	for (int i = 0; i < count; ++i) pairs.emplace_back(i, -i);
	
	timeReadConcurrently(num_threads, pairs);
	timeReadsSerially(num_threads, pairs);
	timeReadsSeriallyAgain(num_threads, pairs);
	timeInsertsConcurrentlyly(num_threads, pairs);
	timeInsertsSerially(num_threads, pairs);
	timeInsertsSeriallyAgain(num_threads, pairs);
}

#endif