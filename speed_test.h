#ifndef SPEED_TEST_H
#define SPEED_TEST_H

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "random_utils.h"
#include "fast_map.h"

enum class Task { READ, WRITE, BALANCED };

template<class T>
std::chrono::high_resolution_clock::rep speed_test(int key_max, int num_threads, int iters, Task task)
{
	std::atomic<int> barrier_1, barrier_2, barrier_3;

	std::chrono::high_resolution_clock::time_point start_time, end_time;

	barrier_1 = 0; barrier_2 = 0; barrier_3 = 0;

	T map;

	auto read_task = [&](int id)
	{
		for (int i = 0; i < iters; ++i)
		{
			int val = random_uint(0, key_max);
			map.insert(std::make_pair(val, -val));
		}

		barrier_1++;
		while (barrier_1 < num_threads) { }
		if (id == 0) start_time = std::chrono::high_resolution_clock::now();
		barrier_2++;
		while (barrier_2 < num_threads) { }

		for (int i = 0; i < iters; ++i)
		{
			map.count(random_uint(0, key_max));
		}

		barrier_3++;
		while (barrier_3 < num_threads) {}
		if (id == 0) end_time = std::chrono::high_resolution_clock::now();
	};

	auto write_task = [&](int id)
	{
		barrier_1++;
		while (barrier_1 < num_threads) { }
		if (id == 0) start_time = std::chrono::high_resolution_clock::now();
		barrier_2++;
		while (barrier_2 < num_threads) { }

		for (int i = 0; i < iters; ++i)
		{
			int val = random_uint(0, key_max);
			if (random_uint(0, 1))
				map.insert(std::make_pair(val, -val));
			else
				map.erase(val);
		}

		barrier_3++;
		while (barrier_3 < num_threads) {}
		if (id == 0) end_time = std::chrono::high_resolution_clock::now();
	};

	auto balanced_task = [&](int id)
	{
		barrier_1++;
		while (barrier_1 < num_threads) { }
		if (id == 0) start_time = std::chrono::high_resolution_clock::now();
		barrier_2++;
		while (barrier_2 < num_threads) { }

		for (int i = 0; i < iters; ++i)
		{
			int action = random_uint(0, 2);
			int val = random_uint(0, key_max);
			if (action == 0)
				map.count(val);
			else if (action == 1)
				map.insert(std::make_pair(val, -val));
			else if (action == 2)
				map.erase(val);
		}

		barrier_3++;
		while (barrier_3 < num_threads) {}
		if (id == 0) end_time = std::chrono::high_resolution_clock::now();
	};

	std::vector<std::thread> threads;

	for (int i = 0; i < num_threads; ++i)
	{
		switch (task)
		{
			case Task::READ:
				threads.push_back(std::thread(read_task, i));
				break;
			case Task::WRITE:
				threads.push_back(std::thread(write_task, i));
				break;
			case Task::BALANCED:
				threads.push_back(std::thread(balanced_task, i));
				break;
		}
	}
	for (int i = 0; i < num_threads; ++i)
		threads[i].join();

	return (end_time - start_time).count();
}

#endif
