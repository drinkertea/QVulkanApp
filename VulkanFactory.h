#pragma once

#include <vulkan/vulkan.h>

#include "VulkanImage.h"

#include <memory>

class QVulkanWindow;

void VkResultSuccess(VkResult res);

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

    virtual Vulkan::Image CreateUploadImage(
        VkFormat format,
        uint32_t width,
        uint32_t height
    ) const = 0;

    virtual Vulkan::Image Create2dImage(
        VkFormat format,
        uint32_t width,
        uint32_t height
    ) const = 0;

    virtual VkSampler CreateSampler() const = 0;

    virtual VkImageView CreateImageView(VkImage image, VkFormat format) const = 0;

    virtual void CopyImage(
        const Vulkan::Image& src,
        const Vulkan::Image& dst,
        const VkExtent3D& extent
    ) const = 0;

    virtual IMappedData::Ptr MapImage(const Vulkan::Image& img) const = 0;

    virtual ~IVulkanFactory() = default;
};
