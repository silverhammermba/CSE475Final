
#include "gtest_include.h"
#include "test_fast_lookup_map.h"
#include "test_fast_map.h"

int main(int argc, char **argv)
{
	FastMap<int, int> fm;
	fm.full_rehash(fm.m_table, fm.m_num_pairs, fm.m_c);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
