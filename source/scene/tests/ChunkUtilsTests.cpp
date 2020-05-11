#include "gtest/gtest.h"

#include "ChunkUtils.h"

#include <array>

using Scene::utils::vec2i;

static constexpr int32_t render_distance = 32;
static constexpr int32_t squere_len = render_distance * 2 + 1;

std::vector<vec2i> GetRenderScope(const vec2i& mid)
{
    std::vector<vec2i> res;
    res.push_back(mid);
    for (int dist = 1; dist < render_distance + 1; ++dist)
    {
        for (int i = dist; i > -dist; --i)
            res.push_back(vec2i{ mid.x + dist, mid.y + i });
        for (int i = dist; i > -dist; --i)
            res.push_back(vec2i{ mid.x + i, mid.y - dist });
        for (int i = -dist; i < dist; ++i)
            res.push_back(vec2i{ mid.x - dist, mid.y + i });
        for (int i = -dist; i < dist; ++i)
            res.push_back(vec2i{ mid.x + i, mid.y + dist });
    }
    return res;
}

std::array<std::array<uint32_t, squere_len>, squere_len> g_valid_indicies = [&]() {
    std::array<std::array<uint32_t, squere_len>, squere_len> res{};
    auto t = GetRenderScope({ 0,0 });
    uint32_t i = 0;
    for (const auto& x : t)
        res[render_distance + x.x][render_distance + x.y] = i++;
    return res;
}();

uint32_t GetValidInd(const vec2i& mid, const vec2i& pos)
{
    auto t = pos - mid;
    return g_valid_indicies[render_distance + t.x][render_distance + t.y];
}

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

TEST(ChunkUtilsTests, get_rank_test)
{
    vec2i cam(-10, 10);
    for (int32_t i = -render_distance; i < render_distance; ++i)
        for (int32_t j = -render_distance; j < render_distance; ++j)
            ASSERT_EQ(
                Scene::utils::GetRank(cam, cam + vec2i(i, j)),
                GetValidInd(cam, cam + vec2i(i, j))
            );
}

TEST(ChunkUtilsTests, ShiftTest1)
{
    std::vector<std::vector<int>> before = {
        { 1, 2 },
        { 4, 5 },
        { 7, 8 },
    };

    std::vector<std::vector<int>> after = {
        { 7, 8 },
        { 0, 0 },
        { 0, 0 },
    };

    Scene::utils::ShiftPlane(vec2i(2, 0), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest2)
{
    std::vector<std::vector<int>> before = {
        { 1, 2 },
        { 1, 2 },
        { 4, 5 },
        { 7, 8 },
    };

    std::vector<std::vector<int>> after = {
        { 4, 5 },
        { 7, 8 },
        { 0, 0 },
        { 0, 0 },
    };

    Scene::utils::ShiftPlane(vec2i(2, 0), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest3)
{
    std::vector<std::vector<int>> before = {
        { 1, 2 },
        { 4, 5 },
        { 7, 8 },
        { 9, 9 }
    };

    std::vector<std::vector<int>> after = {
        { 0, 0 },
        { 0, 0 },
        { 1, 2 },
        { 4, 5 },
    };

    Scene::utils::ShiftPlane(vec2i(-2, 0), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest4)
{
    std::vector<std::vector<int>> before = {
        { 1, 2 },
        { 4, 5 },
        { 7, 8 },
    };

    std::vector<std::vector<int>> after = {
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
    };

    Scene::utils::ShiftPlane(vec2i(30, 0), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest5)
{
    std::vector<std::vector<int>> before = {
        { 1, 2 },
        { 4, 5 },
        { 7, 8 },
    };

    std::vector<std::vector<int>> after = {
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
    };

    Scene::utils::ShiftPlane(vec2i(-30, 0), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest6)
{
    std::vector<std::vector<int>> before = {
        { 8, 1, 2, 3 },
        { 8, 4, 5, 6 },
    };

    std::vector<std::vector<int>> after = {
        { 2, 3, 0, 0 },
        { 5, 6, 0, 0 },
    };

    Scene::utils::ShiftPlane(vec2i(0, 2), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest7)
{
    std::vector<std::vector<int>> before = {
        { 1, 2, 3, 3 },
        { 4, 5, 6, 3 },
    };

    std::vector<std::vector<int>> after = {
        { 0, 0, 1, 2, },
        { 0, 0, 4, 5, },
    };

    Scene::utils::ShiftPlane(vec2i(0, -2), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest8)
{
    std::vector<std::vector<int>> before = {
        { 1, 2, 3, 3 },
        { 4, 5, 6, 3 },
    };

    std::vector<std::vector<int>> after = {
        { 0, 0, 0, 0, },
        { 0, 0, 0, 0, },
    };

    Scene::utils::ShiftPlane(vec2i(0, -20), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest9)
{
    std::vector<std::vector<int>> before = {
        { 1, 2, 3, 3 },
        { 4, 5, 6, 3 },
    };

    std::vector<std::vector<int>> after = {
        { 0, 0, 0, 0, },
        { 0, 0, 0, 0, },
    };

    Scene::utils::ShiftPlane(vec2i(0, 20), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest10)
{
    std::vector<std::vector<int>> before = {
        { 11, 12, 13 },
        { 21, 22, 23 },
        { 31, 32, 33 },
        { 41, 42, 43 },
    };

    std::vector<std::vector<int>> after = {
        { 32, 33, 0 },
        { 42, 43, 0 },
        { 0,  0,  0 },
        { 0,  0,  0 },
    };

    Scene::utils::ShiftPlane(vec2i(2, 1), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest11)
{
    std::vector<std::vector<int>> before = {
        { 11, 12, 13 },
        { 21, 22, 23 },
        { 31, 32, 33 },
        { 41, 42, 43 },
    };

    std::vector<std::vector<int>> after = {
        { 0,  0,  0 },
        { 0, 11, 12 },
        { 0, 21, 22 },
        { 0, 31, 32 },
    };

    Scene::utils::ShiftPlane(vec2i(-1, -1), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest12)
{
    std::vector<std::vector<int>> before = {
        { 11, 12, 13, 14, 15 },
        { 21, 22, 23, 24, 25 },
        { 31, 32, 33, 34, 35 },
        { 41, 42, 43, 44, 45 },
    };

    std::vector<std::vector<int>> after = {
        {  0,  0, 0, 0, 0, },
        { 14, 15, 0, 0, 0, },
        { 24, 25, 0, 0, 0, },
        { 34, 35, 0, 0, 0, },
    };

    Scene::utils::ShiftPlane(vec2i(-1, 3), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest13)
{
    std::vector<std::vector<int>> before = {
        { 11, 12, 13, 14, 15 },
        { 21, 22, 23, 24, 25 },
        { 31, 32, 33, 34, 35 },
        { 41, 42, 43, 44, 45 },
    };

    std::vector<std::vector<int>> after = {
        { 0, 0, 0, 0, 41, },
        { 0, 0, 0, 0,  0, },
        { 0, 0, 0, 0,  0, },
        { 0, 0, 0, 0,  0, },
    };

    Scene::utils::ShiftPlane(vec2i(3, -4), before);
    EXPECT_EQ(before, after);
}

TEST(ChunkUtilsTests, ShiftTest14)
{
    std::vector<std::vector<int>> before = {
        { 11, 12, 13, 14, 15 },
        { 21, 22, 23, 24, 25 },
        { 31, 32, 33, 34, 35 },
        { 41, 42, 43, 44, 45 },
    };

    std::vector<std::vector<int>> after = {
        { 0, 0, 0, 0,  0, },
        { 0, 0, 0, 0,  0, },
        { 0, 0, 0, 0,  0, },
        { 0, 0, 0, 0,  0, },
    };

    Scene::utils::ShiftPlane(vec2i(4, -5), before);
    EXPECT_EQ(before, after);
}

