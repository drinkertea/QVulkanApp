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

