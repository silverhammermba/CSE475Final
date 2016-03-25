#include "test.h"
#include "fastmap.h"

class APerfectTable : public ::testing::Test
{
public:
	PerfectTable m_table;
};

TEST_F(APerfectTable, IsEmptyWhenCreated)
{
	EXPECT_EQ(0u, m_table.size());
}

TEST_F(APerfectTable, CannotAccessMissingElement)
{
	ktype k {5};
	EXPECT_EQ(0u, m_table.count(k));
	EXPECT_THROW(m_table.at(k), std::out_of_range);
}

TEST_F(APerfectTable, CanInsertElement)
{
	ktype k {5};
	vtype v {6};
	ASSERT_EQ(true, m_table.insert(std::make_pair(k, v)));
	EXPECT_EQ(1u, m_table.size());
	EXPECT_EQ(1u, m_table.count(k));
	EXPECT_EQ(v, m_table.at(k));
}

TEST_F(APerfectTable, CanEraseElement)
{
	ktype k {5};
	ASSERT_EQ(0u, m_table.erase(k));

	m_table.insert(std::make_pair(k, k + 1));

	EXPECT_EQ(1u, m_table.erase(k));
	EXPECT_EQ(0u, m_table.count(k));
	EXPECT_EQ(0u, m_table.size());
}
