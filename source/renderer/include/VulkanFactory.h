#pragma once

#include <vulkan/vulkan.h>

#include "IRenderer.h"
#include <memory>

class QVulkanWindow;

namespace Vulkan
{

std::unique_ptr<IFactory> CreateRenderFactory(const QVulkanWindow& window);

struct Buffer
{
    VkBuffer       buffer = nullptr;
    VkDeviceMemory memory = nullptr;
    uint32_t       size   = 0u;
};

struct IMappedData
{
    using bytes = uint8_t*;
    using Ptr = std::unique_ptr<IMappedData>;

    virtual const bytes& GetData() const = 0;
    virtual ~IMappedData() = default;
};

struct IVulkanFactory
{
    static std::unique_ptr<IVulkanFactory> Create(const QVulkanWindow& window);

    virtual Buffer CreateBuffer(
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memory_property,
        VkDeviceSize size,
        void* data
    ) const = 0;

    virtual ~IVulkanFactory() = default;
};



}
