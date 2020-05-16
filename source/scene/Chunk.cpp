#include <DataProvider.h>
#include <IFactory.h>

#include <Noise.h>

#include "Chunk.h"
#include "Texture.h"
#include "IResourceLoader.h"
#include "ThreadUtils.hpp"

namespace Scene
{

constexpr int32_t g_chunk_size = 32;
constexpr uint32_t g_grass_bottom = 57;

Vulkan::Attributes g_instance_attributes = {
    Vulkan::AttributeFormat::vec3f,
    Vulkan::AttributeFormat::vec1i,
    Vulkan::AttributeFormat::vec1i,
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
    uint32_t texture;
    CubeFace face;
};

CubeInstance CreateFace(int32_t x, int32_t y, int32_t z, CubeFace face)
{
    CubeInstance cube = {};
    cube.pos[0] = static_cast<float>(x);
    cube.pos[1] = static_cast<float>(y);
    cube.pos[2] = static_cast<float>(z);
    cube.face = static_cast<CubeFace>(face);
    if (y > 84)
        cube.texture = static_cast<uint32_t>(TextureType::Snow);
    else if (y > 78)
        cube.texture = static_cast<uint32_t>(TextureType::Stone);
    else if (y > g_grass_bottom)
        cube.texture = face == CubeFace::top ?
        static_cast<uint32_t>(TextureType::GrassBlockTop) :
        static_cast<uint32_t>(TextureType::GrassBlockSide);
    else
        cube.texture = static_cast<uint32_t>(TextureType::Sand);
    return cube;
}

Chunk::Chunk(const utils::vec2i& base, Vulkan::IFactory& factory, INoise& noiser, utils::DefferedExecutor& pool)
    : base_point(base)
    , task_queue(pool)
    , frame_buffer_count(factory.GetFrameBufferCount())
{
    auto size = g_chunk_size;
    bbox.first.x = base_point.x * size;
    bbox.first.z = base_point.y * size;
    bbox.first.y = std::numeric_limits<decltype(bbox.first.y)>::max();
    bbox.second.x = base_point.x * size + size;
    bbox.second.z = base_point.y * size + size;
    bbox.second.y = 0;

    std::vector<CubeInstance> cubes;
    std::vector<CubeInstance> water;
    for (int32_t x_offset = 0; x_offset < size; ++x_offset)
    {
        for (int32_t z_offset = 0; z_offset < size; ++z_offset)
        {
            auto x = base_point.x * size + x_offset;
            auto z = base_point.y * size + z_offset;
            int32_t y = noiser.GetHeight(x, z);
            cubes.emplace_back(CreateFace(x, y, z, CubeFace::top));

            if (y < g_grass_bottom)
            {
                water.emplace_back(cubes.back());
                water.back().texture = static_cast<uint32_t>(TextureType::WaterOverlay);
                water.back().pos[1] = static_cast<float>(g_grass_bottom);
            }

            bbox.second.y = std::max(bbox.second.y, std::max(y + 1, static_cast<int32_t>(g_grass_bottom)));
            while (y >= 0)
            {
                auto before = cubes.size();
                if (noiser.GetHeight(x, z + 1) < y)
                {
                    cubes.emplace_back(CreateFace(x, y, z, CubeFace::front));
                }
                if (noiser.GetHeight(x, z - 1) < y)
                {
                    cubes.emplace_back(CreateFace(x, y, z, CubeFace::back));
                }
                if (noiser.GetHeight(x + 1, z) < y)
                {
                    cubes.emplace_back(CreateFace(x, y, z, CubeFace::right));
                }
                if (noiser.GetHeight(x - 1, z) < y)
                {
                    cubes.emplace_back(CreateFace(x, y, z, CubeFace::left));
                }

                if (before == cubes.size())
                    break;

                --y;
            }
            bbox.first.y = std::min(bbox.first.y, y);
        }
    }
    if (cubes.empty())
        cubes.emplace_back(CreateFace(0, 0, 0, CubeFace::front));

    has_water = !water.empty();
    water_offset = static_cast<uint32_t>(cubes.size());
    cubes.insert(cubes.end(), water.begin(), water.end());
    buffer_size = static_cast<uint32_t>(cubes.size());

    alive_marker = task_queue.Add(utils::DefferedExecutor::immediate,
        std::bind([this, &factory](const auto& cubes, const auto& inst_attribs) {
            buffer = factory.CreateInstanceBuffer(Vulkan::BufferDataOwner<CubeInstance>(cubes), inst_attribs);
        }, std::move(cubes), g_instance_attributes)
    );
}

Chunk::~Chunk()
{
    if (!alive_marker.expired())
        *alive_marker.lock() = false;

    if (!buffer)
        return;

    std::shared_ptr<Vulkan::IInstanceBuffer> to_release = std::move(buffer);
    task_queue.Add(frame_buffer_count, [bp = to_release]() {});
}

const Vulkan::IInstanceBuffer& Scene::Chunk::GetData() const
{
    return *buffer;
}

const std::pair<Point3D, Point3D>& Chunk::GetBBox() const
{
    return bbox;
}

utils::vec2i WorldToChunk(const utils::vec2i& pos)
{
    return { pos.x / g_chunk_size, pos.y / g_chunk_size };
}

}
