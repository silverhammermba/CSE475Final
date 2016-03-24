#include "fastmap.h"

#ifdef TESTING

#include "test.h"
#include "test_fastmap.h"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

#else

#include "fastmap.h"

int main(int, char**)
{
	try
	{
		PerfectTable table;
		table.insert(std::make_pair(3, 6));
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

#endif
