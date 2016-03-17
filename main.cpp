#include "fastmap.h"

int main(int, char**)
{
	try
	{
		STable table(std::make_pair(3, 6));
		auto c = table.count(3);
		auto c2 = table.count(4);
		auto v = table.at(3);
		auto b = table.erase(3);
		auto b2 = table.erase(3);
	}
	catch (...)
	{
		int i = 0;
	}


	return EXIT_SUCCESS;
}
