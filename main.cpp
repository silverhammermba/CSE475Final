
#include "gtest_include.h"
#include "test_fast_map.h"
#include "test_perfect_table.h"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
