#pragma once

#include <vector>
#include <map>
#include <array>
#include <mutex>

#include "ChunkUtils.h"

namespace Vulkan
{

struct IFactory;

}

struct INoise;

namespace Scene
{

namespace utils
{

struct DefferedExecutor;

}

struct Point3D
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

utils::vec2i WorldToChunk(const utils::vec2i& pos);

struct Chunk
{
    Chunk(const utils::vec2i& base, Vulkan::IFactory& factory, INoise& noiser, utils::DefferedExecutor& pool);
    ~Chunk();

    const Vulkan::IInstanceBuffer& GetData() const;

    const std::pair<Point3D, Point3D>& GetBBox() const;

    bool Ready() const { return !!buffer; }

private:
    utils::vec2i                base_point{};
    std::pair<Point3D, Point3D> bbox;

    std::unique_ptr<Vulkan::IInstanceBuffer> buffer;

    utils::DefferedExecutor& task_queue;
    std::weak_ptr<bool>      alive_marker;
    uint32_t                 frame_buffer_count = 1u;
};

using ChunkPtr = std::unique_ptr<Chunk>;

}
