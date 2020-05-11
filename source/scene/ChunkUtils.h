#pragma once
#include <tuple>

namespace Scene
{

template <typename T>
struct vec2
{
    T x{};
    T y{};

    bool operator<(const vec2<T>& r) const
    {
        return std::tie(x, y) < std::tie(r.x, r.y);
    }

    bool operator==(const vec2<T>& r) const
    {
        return std::tie(x, y) == std::tie(r.x, r.y);
    }

    bool operator!=(const vec2<T>& r) const
    {
        return !(*this == r);
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

struct IChunkStorage
{


    virtual ~IChunkStorage() = default;
};

}
