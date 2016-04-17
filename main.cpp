#include "fast_lookup_map_timing.h"
#include "gtest_include.h"
#include "test_fast_lookup_map.h"
#include "test_fast_map.h"

// TODO:	Figure out constants c and s(M),M relationship
//			Make full_rebuild FastLookupMap creation more memory efficient
//			When # partitions decreases, is it better to reduce memory allocation or to track separate partition count
//			Delete - add full_rebuild logic

int main(int argc, char **argv)
{
	runFastLookupMapTiming();

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
