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
constexpr uint32_t g_grass_top = 78;



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

CubeInstance CreateFace(int32_t x, int32_t y, int32_t z, CubeFace face, TextureType type)
{
    CubeInstance cube = {};
    cube.pos[0] = static_cast<float>(x);
    cube.pos[1] = static_cast<float>(y);
    cube.pos[2] = static_cast<float>(z);
    cube.face = static_cast<CubeFace>(face);
    cube.texture = static_cast<uint32_t>(type);
    return cube;
}

CubeInstance CreateFace(int32_t x, int32_t y, int32_t z, CubeFace face)
{
    if (y > 84)
        return CreateFace(x,y, z, face, TextureType::Snow);
    else if (y > g_grass_top)
        return CreateFace(x, y, z, face, TextureType::Stone);
    else if (y > g_grass_bottom)
        return CreateFace(x, y, z, face, face == CubeFace::top ? TextureType::GrassBlockTop :TextureType::GrassBlockSide);

    return CreateFace(x, y, z, face, TextureType::Sand);
}

void AddTree(int32_t x, int32_t y, int32_t z, std::vector<CubeInstance>& cubes)
{
    constexpr int32_t height = 5;
    constexpr int32_t rad = 2;
    for (int i = 0; i < height; ++i)
    {
        cubes.emplace_back(CreateFace(x, y + i, z, CubeFace::front, TextureType::OakLog));
        cubes.emplace_back(CreateFace(x, y + i, z, CubeFace::back, TextureType::OakLog));
        cubes.emplace_back(CreateFace(x, y + i, z, CubeFace::left, TextureType::OakLog));
        cubes.emplace_back(CreateFace(x, y + i, z, CubeFace::right, TextureType::OakLog));
    }

    for (int i = -rad; i <= rad; ++i)
    {
        for (int j = -rad; j <= rad; ++j)
        {
            for (int k = 0; k < 3; ++k)
            {
                if (k == 2 && (std::abs(i) > 1 || std::abs(j) > 1))
                    continue;
                cubes.emplace_back(CreateFace(x + i, y + 3 + k, z + j, CubeFace::front, TextureType::LeavesOakOpaque));
                cubes.emplace_back(CreateFace(x + i, y + 3 + k, z + j, CubeFace::back,  TextureType::LeavesOakOpaque));
                cubes.emplace_back(CreateFace(x + i, y + 3 + k, z + j, CubeFace::left,  TextureType::LeavesOakOpaque));
                cubes.emplace_back(CreateFace(x + i, y + 3 + k, z + j, CubeFace::right, TextureType::LeavesOakOpaque));
            }

            int add = !(std::abs(i) > 1 || std::abs(j) > 1);
            cubes.emplace_back(CreateFace(x + i, y + 3, z + j, CubeFace::bottom, TextureType::LeavesOakOpaque));
            cubes.emplace_back(CreateFace(x + i, y + 4 + add, z + j, CubeFace::top, TextureType::LeavesOakOpaque));

            if (i == x && j == z)
                continue;
        }
    }

    cubes.emplace_back(CreateFace(x, y + height - 1, z, CubeFace::top, TextureType::OakLogTop));

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
            if (noiser.IsTree(x, z) && cubes.back().texture == static_cast<uint32_t>(TextureType::GrassBlockTop))
            {
                //cubes.back().texture = static_cast<uint32_t>(TextureType::OakLog);
                AddTree(x, y, z, cubes);
            }


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
        std::bind([this, &factory](const auto& cubes) {
            buffer = factory.CreateBuffer(Vulkan::BufferUsage::Instance, Vulkan::BufferDataOwner<CubeInstance>(cubes));
        }, std::move(cubes))
    );
}

Chunk::~Chunk()
{
    auto apive_sp = alive_marker.lock();
    if (apive_sp)
        *apive_sp = false;

    if (!buffer)
        return;

    std::shared_ptr<Vulkan::IBuffer> to_release = std::move(buffer);
    task_queue.Add(frame_buffer_count, [bp = to_release]() {});
}

const Vulkan::IBuffer& Scene::Chunk::GetData() const
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
