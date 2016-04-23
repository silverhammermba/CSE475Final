#include "gtest_include.h"
#include "test_fast_lookup_map.h"
#include "test_fast_map.h"
#include "config_t.h"
#include "tests.h"

// TODO:	Figure out constants c and s(M),M relationship
//			When # partitions decreases, is it better to reduce memory allocation or to track separate partition count

int main(int argc, char **argv)
{
	config_t cfg;
	test_driver(cfg);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
