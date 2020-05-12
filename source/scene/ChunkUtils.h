#pragma once
#include <tuple>
#include <cstdint>
#include <vector>
#include <functional>

namespace Scene
{

namespace utils
{

template <typename T>
struct vec2
{
    T x;
    T y;

    vec2(T x = {}, T y = {})
        : x(x)
        , y(y)
    {
    }

    constexpr friend inline const bool operator<(const vec2<T>& v1, const vec2<T>& v2)
    {
        return std::tie(v1.x, v1.y) < std::tie(v2.x, v2.y);
    }

    constexpr friend inline const bool operator==(const vec2<T>& v1, const vec2<T>& v2)
    {
        return std::tie(v1.x, v1.y) == std::tie(v2.x, v2.y);
    }

    constexpr friend inline const bool operator!=(const vec2<T>& v1, const vec2<T>& v2)
    {
        return !(v1 == v2);
    }

    constexpr friend inline const vec2<T> operator+(const vec2<T>& v1, const vec2<T>& v2)
    {
        return vec2<T>(v1.x + v2.x, v1.y + v2.y);
    }

    constexpr friend inline const vec2<T> operator-(const vec2<T>& v1, const vec2<T>& v2)
    {
        return vec2<T>(v1.x - v2.x, v1.y - v2.y);
    }
};

using vec2i = vec2<int32_t>;

uint32_t GetRank(const vec2i& mid, const vec2i& pos);

void IterateFromMid(int distance, const utils::vec2i& mid, const std::function<void(int, const utils::vec2i&)>& callback);

template <typename T>
void ShiftPlane(const vec2i& translation, std::vector<std::vector<T>>& plane)
{
    if (plane.empty())
        return;

    int n = static_cast<int>(plane.size());
    int m = static_cast<int>(plane.front().size());
    int x_from = 0;
    int x_to = n;
    if (translation.x > 0)
    {
        for (int i = 0; i < std::min(translation.x, n); ++i)
        {
            for (auto& x : plane[i])
                x = {};
        }
        for (int i = 0; i + translation.x < n; ++i)
        {
            std::swap(plane[i], plane[i + translation.x]);
        }
        x_to = n - translation.x;

    }
    else if (translation.x < 0)
    {
        for (int i = n - 1; i >= -translation.x; --i)
        {
            std::swap(plane[i], plane[i + translation.x]);
        }
        x_from = std::min(-translation.x, n);
        for (int i = 0; i < x_from; ++i)
        {
            for (auto& x : plane[i])
                x = {};
        }
    }
    if (translation.y < 0)
    {
        for (int k = x_from; k < x_to; ++k)
        {
            for (int i = m - 1; i >= -translation.y; --i)
            {
                std::swap(plane[k][i], plane[k][i + translation.y]);
            }
            for (int i = 0; i < std::min(-translation.y, m); ++i)
            {
                plane[k][i] = {};
            }
        }
    }
    else if (translation.y > 0)
    {
        for (int k = x_from; k < x_to; ++k)
        {
            for (int i = 0; i < std::min(translation.y, m); ++i)
            {
                plane[k][i] = {};
            }
            for (int i = 0; i + translation.y < m; ++i)
            {
                std::swap(plane[k][i], plane[k][i + translation.y]);
            }
        }
    }
}

}

}
