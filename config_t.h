#include <iostream>
#include <string>

// stores our command-line config parameters
struct config_t
{
	std::string name; // string to identify test run
	int key_max;      // maximum key size
	int threads;      // number of threads to use
	int iters;        // how many operations each thread performs

	// simple constructor
	config_t()
		: key_max(2560), iters(10000), name("no_name"), threads(1)
	{}

	// Print the values of the seed, iters, and name fields
	void dump()
	{
		std::cout << "# name, key_max, iters, threads, time" << std::endl
		          << name << ", "
		          << key_max << ", "
		          << iters << ", "
		          << threads << ", ";
	}
};
