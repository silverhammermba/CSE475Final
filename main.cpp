#include <iostream>
#include <boost/program_options.hpp>

#include "gtest_include.h"
#include "test_fast_lookup_map.h"
#include "test_fast_map.h"
#include "speed_test.h"

// TODO: Figure out constants c and s(M),M relationship
//       When # partitions decreases, is it better to reduce memory allocation or to track separate partition count
namespace po = boost::program_options;

int main(int argc, char** argv)
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "print this message and exit")
		("unit,u", "run unit tests instead")
		("key-max,k", po::value<int>()->required(), "upper bound of random keys")
		("threads,t", po::value<int>()->required(), "number of threads")
		("iters,i", po::value<int>()->required(), "number of iterations per-thread")
	;

	po::variables_map options;
	po::store(po::parse_command_line(argc, argv, desc), options);

	if (options.count("help"))
	{
		std::cerr << desc << std::endl;
		return EXIT_FAILURE;
	}

	if (options.count("unit"))
	{
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
	}

	po::notify(options);

	std::cout << speed_test<FastMap<int, int>>(
		options["key-max"].as<int>(),
		options["threads"].as<int>(),
		options["iters"].as<int>()
	) << std::endl;

	return EXIT_SUCCESS;
}
