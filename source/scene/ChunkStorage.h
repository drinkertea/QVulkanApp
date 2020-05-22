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

struct Chunk;

struct IChunkStorage
{
    virtual void OnRender() = 0;

    virtual void ForEach(const std::function<void(const Chunk&)>& callback) = 0;

    virtual ~IChunkStorage() = default;

    static std::unique_ptr<IChunkStorage> Create(Vulkan::ICamera& camera, Vulkan::IFactory& factory);
};

}

