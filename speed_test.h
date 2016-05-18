#ifndef SPEED_TEST_H
#define SPEED_TEST_H

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "random_utils.h"

template<class T>
std::chrono::high_resolution_clock::rep speed_test(int key_max, int num_threads, int iters, int reads, int writes, int erases, int prepop)
{
	std::atomic<int> barrier_1, barrier_2, barrier_3;

	std::chrono::high_resolution_clock::time_point start_time, end_time;

	barrier_1 = 0; barrier_2 = 0; barrier_3 = 0;

	T map;
	// pre-populate map (non-randomly because it doesn't matter)
	for (int i = 0; i < prepop && i <= key_max; ++i) map.insert(std::make_pair(i, -i));

	int ops = reads + writes + erases - 1;
	writes += reads;

	auto task = [&](int id)
	{
		barrier_1++;
		while (barrier_1 < num_threads) { }
		if (id == 0) start_time = std::chrono::high_resolution_clock::now();
		barrier_2++;
		while (barrier_2 < num_threads) { }

		for (int i = 0; i < iters; ++i)
		{
			int action = random_uint(0, ops);
			int val = random_uint(0, key_max);
			if (action < reads)
				map.count(val);
			else if (action < writes)
				map.insert(std::make_pair(val, -val));
			else
				map.erase(val);
		}

		barrier_3++;
		while (barrier_3 < num_threads) {}
		if (id == 0) end_time = std::chrono::high_resolution_clock::now();
	};

	std::vector<std::thread> threads;

	for (int i = 0; i < num_threads; ++i) threads.push_back(std::thread(task, i));
	for (int i = 0; i < num_threads; ++i) threads[i].join();

	return (end_time - start_time).count();
}

#endif
