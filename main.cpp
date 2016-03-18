#include <iostream>

#include "fastmap.h"

int main(int, char**)
{
	try
	{
		PerfectTable table;
		std::cout << table.size() << std::endl;

		std::cout << table.count(3) << std::endl;
		table.insert(std::make_pair(3, 6));
		std::cout << table.size() << std::endl;

		std::cout << table.count(3) << std::endl;
		std::cout << table.at(3) << std::endl;
		std::cout << table.erase(3) << std::endl;
		std::cout << table.erase(3) << std::endl;
		std::cout << table.count(3) << std::endl;
	}
	catch (...)
	{
		std::cout << "error\n";
	}

	for (int i = 0; i < 10; ++i)
		std::cout << random_prime_at_least(2) << std::endl;

	return EXIT_SUCCESS;
}
