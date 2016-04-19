// CSE 375/475 Assignment #3
// Spring 2016

// Description: This file implements a function that should be able to use
// the configuration information to drive tests that evaluate the correctness
// and performance of your list(s) and allocator(s)

#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include "config_t.h"
#include "tests.h"
#include "fast_map.h"

namespace 
{
	int getRand(unsigned int *seed)
	{
		#ifdef __linux__
			return rand_r(seed);
		#elif _WIN32
			seed; // satisfies unused arg compiler warning
			return rand();
		#endif
	}

	#ifdef __linux__
		thread_local unsigned seed; // for use with rand_r
	#endif

	template<class T>
	void run_custom_tests(config_t& cfg) {
		std::atomic<int> barrier_1, barrier_2, barrier_3;

		std::chrono::high_resolution_clock::time_point start_time, end_time;

		barrier_1 = 0; barrier_2 = 0; barrier_3 = 0;

		auto task = [&](int id)
		{
			#ifdef __linux__
				seed = id;
			#elif _WIN32
				unsigned seed = id;
				srand(seed);
			#endif

			T map;

			barrier_1++;
			while (barrier_1 < cfg.threads) { }
			if (id == 0)
				start_time = std::chrono::high_resolution_clock::now();
			barrier_2++;
			while (barrier_2 < cfg.threads) { }

			for (int i = 0; i < cfg.iters; ++i) {
				int action = getRand(&seed) % 3;
				int val = getRand(&seed) % cfg.key_max;
				if (action == 0)
					map.count(val);
				else if (action == 1)
					map.insert(std::make_pair(val, -val));
				else if (action == 2)
					map.erase(val);
			}

			barrier_3++;
			while (barrier_3 < cfg.threads) {}
			if (id == 0)
				end_time = std::chrono::high_resolution_clock::now();
		};

		std::vector<std::thread> threads;

		for (int i = 0; i < cfg.threads; ++i)
			threads.push_back(std::thread(task, i));
		for (int i = 0; i < cfg.threads; ++i)
			threads[i].join();

		std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << std::endl;
	}

} // end anonymous namespace

void test_driver(config_t &cfg)
{
	if (cfg.map == "multithreaded_rev1")	run_custom_tests<FastMap<int, int>>(cfg);

}
