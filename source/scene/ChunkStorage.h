#pragma once

#include <functional>
#include <memory>

namespace Vulkan
{

struct ICamera;
struct IFactory;
struct IInstanceBuffer;

}

namespace Scene
{

class Chunk;

struct IChunkStorage
{
    virtual void DoCpuWork() = 0;
    virtual void DoGpuWork() = 0;

    virtual void ForEach(const std::function<void(const Chunk&)>& callback) = 0;

    virtual const Vulkan::IInstanceBuffer& GetInstanceLayout() const = 0;

    virtual ~IChunkStorage() = default;

    static std::unique_ptr<IChunkStorage> Create(Vulkan::ICamera& camera, Vulkan::IFactory& factory);
};

}

