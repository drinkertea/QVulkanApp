#include "ChunkUtils.h"

#include <algorithm>

namespace Scene
{

namespace utils
{

uint32_t GetRank(const vec2i& mid, const vec2i& pos)
{
    if (mid == pos)
        return 0;

    int32_t x = pos.x - mid.x;
    int32_t y = pos.y - mid.y;
    int32_t d = std::max(std::abs(x), std::abs(y));
    int32_t o = 0;

    if (x == d)
        o = d - y;
    else if (y == -d)
        o = d * 3 - x;
    else if (x == -d)
        o = d * 5 + y;
    else
        o = d * 7 + x;
    return static_cast<uint32_t>((d * 2 - 1)* (d * 2 - 1) + o);
}

}

}
