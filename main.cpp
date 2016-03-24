
//#define APPLICATION
//#define GMOCK_TESTING

#if defined(APPLICATION)

#include "fastmap.h"

int main(int, char**)
{
	try
	{
		PerfectTable table(std::make_pair(3, 6));
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

#elif defined(GMOCK_TESTING)

#include "fastmap.h"
#include "my_gmock.h"

using namespace ::testing;

int main(int argc, char **argv)
{
	testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}

#endif