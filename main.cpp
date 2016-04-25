#include <boost/program_options.hpp>

#include "gtest_include.h"
#include "test_fast_lookup_map.h"
#include "test_fast_map.h"
#include "config_t.h"
#include "tests.h"

// TODO:	Figure out constants c and s(M),M relationship
//			When # partitions decreases, is it better to reduce memory allocation or to track separate partition count
namespace po = boost::program_options;

int main(int argc, char** argv)
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "print this message and exit")
		("unit,u", "run unit tests instead")
		("name,n", po::value<std::string>()->default_value("-"), "name of test run")
		("key-max,k", po::value<int>()->default_value(2500), "upper bound of random keys")
		("threads,t", po::value<int>()->default_value(1), "number of threads")
		("iters,i", po::value<int>()->default_value(1000), "number of iterations per-thread")
	;

	po::variables_map options;
	po::store(po::parse_command_line(argc, argv, desc), options);
	po::notify(options);

	if (options.count("help"))
	{
		std::cerr << desc << std::endl;
		return 1;
	}

	if (options.count("unit"))
	{
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
	}

	config_t cfg(
		options["name"].as<std::string>(),
		options["key-max"].as<int>(),
		options["threads"].as<int>(),
		options["iters"].as<int>()
	);
	test_driver(cfg);

	return EXIT_SUCCESS;
}
