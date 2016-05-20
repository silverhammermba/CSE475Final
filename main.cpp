#include <iostream>
#include <boost/program_options.hpp>

#include "speed_test.h"
#include "fast_map.h"

// TODO: Figure out constants c and s(M),M relationship
//       When # partitions decreases, is it better to reduce memory allocation or to track separate partition count
namespace po = boost::program_options;

int main(int argc, char** argv)
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "print this message and exit")
		("key-max,k", po::value<int>()->required(), "upper bound of random keys")
		("threads,t", po::value<int>()->required(), "number of threads")
		("iters,i", po::value<int>()->required(), "number of iterations per-thread")
		("read,r", po::value<int>()->default_value(1), "proportion of reads in speed test")
		("write,w", po::value<int>()->default_value(1), "proportion of writes in speed test")
		("erase,e", po::value<int>()->default_value(1), "proportion of erases in speed test")
		("pop,p", po::value<int>()->default_value(0), "initial number of inserts before speed test")
	;

	po::variables_map options;

	try
	{
		po::store(po::parse_command_line(argc, argv, desc), options);

		if (options.count("help"))
		{
			std::cerr << desc << std::endl;
			return EXIT_FAILURE;
		}

		po::notify(options);

		std::cout << speed_test<FastMap<int, int>>(
			options["key-max"].as<int>(),
			options["threads"].as<int>(),
			options["iters"].as<int>(),
			options["read"].as<int>(),
			options["write"].as<int>(),
			options["erase"].as<int>(),
			options["pop"].as<int>()
		) << std::endl;
	}
	catch (const po::error& e)
	{
		std::cerr << e.what() << std::endl << desc << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
