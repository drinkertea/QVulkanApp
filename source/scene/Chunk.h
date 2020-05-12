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
    uint32_t texture;
    CubeFace face;
};

class TaskQueue
{
public:
    uint64_t AddTask(std::function<void()>&& task);
    void AddRelTask(std::function<void()>&& task);
    void DelTask(uint64_t);
    void ExecuteAll();
    ~TaskQueue();

private:
    using TaskPool = std::map<uint64_t, std::function<void()>>;

    uint64_t    curr_task_id = 0;
    TaskPool    tasks;
    TaskPool    rel_tasks;

    std::map<uint64_t, uint64_t> rel_attempts;

    std::mutex  mutex;
};

class Chunk
{
    static constexpr int32_t size = 32;

public:
    Chunk(const utils::vec2i& base, Vulkan::IFactory& factory, INoise& noiser, TaskQueue& pool);
    ~Chunk();

    const Vulkan::IInstanceBuffer& GetData() const;

    const std::pair<Point3D, Point3D>& GetBBox() const;

    operator bool() const { return !!buffer; }

    const utils::vec2i& GetBase() const { return base_point; }

    static utils::vec2i GetChunkBase(const utils::vec2i& pos);

private:
    utils::vec2i base_point{};
    std::pair<Point3D, Point3D> bbox;

    TaskQueue& task_queue;
    uint64_t create_task_id;

    std::unique_ptr<Vulkan::IInstanceBuffer> buffer;
};

}
