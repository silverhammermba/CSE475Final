#include <iostream>
#include <boost/program_options.hpp>
#include <boost\date_time\posix_time\posix_time.hpp>
#include <fstream>
#include "gtest_include.h"
#include "test_fast_lookup_map.h"
#include "test_fast_map.h"
#include "speed_test.h"

// TODO: Figure out constants c and s(M),M relationship
//       When # partitions decreases, is it better to reduce memory allocation or to track separate partition count
namespace po = boost::program_options;

// parse what kind of task to run
std::istream& operator>>(std::istream& in, Task& task)
{
	std::string token;
	in >> token;
	if (token == "r")
		task = Task::READ;
	else if (token == "w")
		task = Task::WRITE;
	else if (token == "b")
		task = Task::BALANCED;
	else
		throw po::validation_error(po::validation_error::invalid_option_value);
	return in;
}

int main(int argc, char** argv)
{
	/*po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "print this message and exit")
		("unit,u", "run unit tests instead")
		("key-max,k", po::value<int>()->required(), "upper bound of random keys")
		("threads,t", po::value<int>()->required(), "number of threads")
		("iters,i", po::value<int>()->required(), "number of iterations per-thread")
		("task,a", po::value<Task>()->required(), "type of task to test r/w/b for reads/write/balanced")
	;

	po::variables_map options;
	po::store(po::parse_command_line(argc, argv, desc), options);

	if (options.count("help"))
	{
		std::cerr << desc << std::endl;
		return EXIT_FAILURE;
	}*/

	/*if (options.count("unit"))
	{
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
	}*/

	//po::notify(options);

	/*std::cout << speed_test<FastMap<int, int>>(
		options["key-max"].as<int>(),
		options["threads"].as<int>(),
		options["iters"].as<int>(),
		options["task"].as<Task>()
	) << std::endl;*/

	auto time = boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());

	std::ofstream ofs(time + "_timing.csv");
	std::vector<int> thread_count{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
	int it = 1000000;
	int keys = 100000;

	for (auto t : thread_count)
	{
		std::cout << t << std::endl;
		ofs << t << "," << speed_test<FastMap<int, int>>(keys, t, it / t, Task::BALANCED) << std::endl;
	}

	return EXIT_SUCCESS;
}
