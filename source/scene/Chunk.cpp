#include "Noise.h"
#include "Chunk.h"

namespace Scene
{

class CubeInstanceData
    : public Vulkan::IDataProvider
{
    const std::vector<CubeInstance>& instances;

public:
    CubeInstanceData(const std::vector<CubeInstance>& inst)
        : instances(inst)
    {
    }
    ~CubeInstanceData() override = default;

    uint32_t GetWidth() const override
    {
        return static_cast<uint32_t>(instances.size());
    }

    uint32_t GetHeight() const override
    {
        return 1;
    }

    const uint8_t* GetData() const override
    {
        return reinterpret_cast<const uint8_t*>(instances.data());
    }

    uint32_t GetSize() const override
    {
        return GetWidth() * sizeof(CubeInstance);
    }

    uint32_t GetDepth() const override
    {
        return 1;
    }
};

CubeInstance CreateFace(int32_t x, int32_t y, int32_t z, CubeFace face)
{
    CubeInstance cube = {};
    cube.pos[0] = static_cast<float>(x);
    cube.pos[1] = static_cast<float>(y);
    cube.pos[2] = static_cast<float>(z);
    cube.face = static_cast<CubeFace>(face);
    if (y > 84)
        cube.texture = 6;
    else if (y > 78)
        cube.texture = 4;
    else if (y > 57)
        cube.texture = face == CubeFace::top ? 8 : 9;
    else
        cube.texture = 5;
    return cube;
}

Chunk::Chunk(const Point2D& base, Vulkan::IFactory& factory, INoise& noiser)
    : base_point(base)
{
    constexpr float amplitude = 0.5f;
    std::vector<CubeInstance> cubes;
    for (int32_t x_offset = 0; x_offset < size; ++x_offset)
    {
        for (int32_t z_offset = 0; z_offset < size; ++z_offset)
        {
            auto x = base_point.x * size + x_offset;
            auto z = base_point.y * size + z_offset;
            int32_t y = noiser.GetHeight(x, z);

            cubes.emplace_back(CreateFace(x, y, z, CubeFace::top));
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
        }
    }
    if (cubes.empty())
        cubes.emplace_back(CreateFace(0, 0, 0, CubeFace::front));

    Vulkan::Attributes inst_attribs;
    inst_attribs.push_back(Vulkan::AttributeFormat::vec3f);
    inst_attribs.push_back(Vulkan::AttributeFormat::vec1i);
    inst_attribs.push_back(Vulkan::AttributeFormat::vec1i);

    buffer = &factory.CreateInstanceBuffer(CubeInstanceData(cubes), inst_attribs);
}

const Vulkan::IInstanceBuffer& Scene::Chunk::GetData() const
{
    return *buffer;
}

}
