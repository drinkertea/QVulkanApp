#pragma once

#include "IFactory.h"

#include <vector>
#include <array>

struct INoise;

namespace Scene
{

struct Point2D
{
    int32_t x = 0;
    int32_t y = 0;
};

struct Point3D
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

enum class CubeFace : uint32_t
{
    front = 0,
    back,
    left,
    right,
    top,
    bottom,
    count,
};

struct CubeInstance
{
    float    pos[3];
    int32_t  texture;
    CubeFace face;
};

class Chunk
{
    static constexpr int32_t size = 32;

public:
    Chunk(const Point2D& base, Vulkan::IFactory& factory, INoise& noiser);

    const Vulkan::IInstanceBuffer& GetData() const;

    const std::pair<Point3D, Point3D>& GetBBox() const;

private:
    Point2D base_point{};
    std::pair<Point3D, Point3D> bbox;

    Vulkan::IInstanceBuffer* buffer = nullptr;
};

}
