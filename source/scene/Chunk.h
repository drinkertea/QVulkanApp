#pragma once

#include <vector>
#include <deque>
#include <array>
#include <mutex>

namespace Vulkan
{

struct IFactory;

}

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
    uint32_t texture;
    CubeFace face;
};


class TaskDeque
{
public:
    void AddTask(std::function<void()>&& task);
    void ExecuteAll();

private:
    using TaskPool = std::deque<std::function<void()>>;

    TaskPool   tasks;
    std::mutex mutex;
};

class Chunk
{
    static constexpr int32_t size = 32;

public:
    Chunk(const Point2D& base, Vulkan::IFactory& factory, INoise& noiser, TaskDeque& pool);

    const Vulkan::IInstanceBuffer& GetData() const;

    const std::pair<Point3D, Point3D>& GetBBox() const;

    operator bool() const { return !!buffer; }

    static Point2D GetChunkBase(const Point2D& pos);

private:
    Point2D base_point{};
    std::pair<Point3D, Point3D> bbox;

    std::unique_ptr<Vulkan::IInstanceBuffer> buffer;
};

}
