#include "gtest/gtest.h"

#include "ChunkUtils.h"

using Scene::utils::vec2i;

TEST(ChunkUtilsTests, add)
{
    ASSERT_EQ(vec2i(3, 4) + vec2i(1, 2), vec2i(4, 6));
}

TEST(ChunkUtilsTests, minus)
{
    ASSERT_EQ(vec2i(3, 4) - vec2i(-1, -2), vec2i(4, 6));
}

TEST(ChunkUtilsTests, less)
{
    ASSERT_TRUE(vec2i(1, 2) < vec2i(3, 4));
    ASSERT_TRUE(vec2i(2, 2) < vec2i(2, 3));
    ASSERT_TRUE(vec2i(2, 2) < vec2i(3, 2));
    ASSERT_TRUE(vec2i(2, 3) < vec2i(3, 2));
    ASSERT_FALSE(vec2i(3, 2) < vec2i(3, 2));
}
