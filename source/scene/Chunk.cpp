#include <DataProvider.h>
#include <IFactory.h>

#include <Noise.h>

#include "Chunk.h"
#include "Texture.h"
#include "IResourceLoader.h"

namespace Scene
{

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
    else if (y > 57)
        cube.texture = face == CubeFace::top ?
        static_cast<uint32_t>(TextureType::GrassBlockTop) :
        static_cast<uint32_t>(TextureType::GrassBlockSide);
    else
        cube.texture = static_cast<uint32_t>(TextureType::Sand);
    return cube;
}

Chunk::Chunk(const Point2D& base, Vulkan::IFactory& factory, INoise& noiser, TaskDeque& pool)
    : base_point(base)
{
    bbox.first.x = base_point.x * size;
    bbox.first.z = base_point.y * size;
    bbox.first.y = std::numeric_limits<decltype(bbox.first.y)>::max();
    bbox.second.x = base_point.x * size + size;
    bbox.second.z = base_point.y * size + size;
    bbox.second.y = 0;

    std::vector<CubeInstance> cubes;
    for (int32_t x_offset = 0; x_offset < size; ++x_offset)
    {
        for (int32_t z_offset = 0; z_offset < size; ++z_offset)
        {
            auto x = base_point.x * size + x_offset;
            auto z = base_point.y * size + z_offset;
            int32_t y = noiser.GetHeight(x, z);
            cubes.emplace_back(CreateFace(x, y, z, CubeFace::top));

            bbox.second.y = std::max(bbox.second.y, y + 1);
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

    Vulkan::Attributes inst_attribs;
    inst_attribs.push_back(Vulkan::AttributeFormat::vec3f);
    inst_attribs.push_back(Vulkan::AttributeFormat::vec1i);
    inst_attribs.push_back(Vulkan::AttributeFormat::vec1i);

    pool.AddTask(std::bind(
        [this, &factory](const std::vector<CubeInstance>& cubes, const Vulkan::Attributes& inst_attribs) {
            buffer = factory.CreateInstanceBuffer(Vulkan::BufferDataOwner<CubeInstance>(cubes), inst_attribs);
        },
        std::move(cubes),
        std::move(inst_attribs)
    ));
}

const Vulkan::IInstanceBuffer& Scene::Chunk::GetData() const
{
    return *buffer;
}

const std::pair<Point3D, Point3D>& Chunk::GetBBox() const
{
    return bbox;
}

Point2D Chunk::GetChunkBase(const Point2D& pos)
{
    return { pos.x / size, pos.y / size };
}

void TaskDeque::AddTask(std::function<void()>&& task)
{
    std::lock_guard<std::mutex> lg(mutex);
    tasks.push_back(std::move(task));
}

void TaskDeque::ExecuteAll()
{
    std::lock_guard<std::mutex> lg(mutex);
    for (auto& task : tasks)
        task();
    tasks.clear();
}

}
