#pragma once
#include <tuple>
#include <cstdint>

namespace Scene
{

namespace utils
{

template <typename T>
struct vec2
{
    T x;
    T y;

    vec2(T&& x = {}, T&& y = {})
        : x(std::forward<T>(x))
        , y(std::forward<T>(y))
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



}

}
