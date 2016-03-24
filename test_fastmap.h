#include "test.h"
#include "fastmap.h"

class APerfectTable : public ::testing::Test
{
public:
	PerfectTable m_perfect_table;
};

TEST_F(APerfectTable, IsEmptyWhenCreated)
{
	EXPECT_EQ(m_perfect_table.size(), 0u);
}

TEST_F(APerfectTable, ContainsNoElementThatWasntAdded)
{
	ktype k{ 5 };
	EXPECT_EQ(m_perfect_table.count(k), 0u);
	EXPECT_THROW(m_perfect_table.at(k), std::exception);
}

//TEST_F(APerfectTable, ContainsTheElementThatWasAdded)
//{
//
//	pair_t p(3, 5);
//	ASSERT_THAT(m_perfect_table.count(, Eq(0u));
//}

	// table has no keys at start
	// COUNT a key that doesn't exist returns zero
// COUNT a key that exists returns one
// AT a key that exists returns value
// AT a key that doesn't exist throws
// ERASE a key that exists removes key, decrements num_keys, and returns one
// ERASE a key that doesn't exist does nothing to vector or num_keys, and returns zero
// INSERT duplicate key and same value returns false
// INSERT duplicate key and different value returns false
// Capacity is twice the num keys
