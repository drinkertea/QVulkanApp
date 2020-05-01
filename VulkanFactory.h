#pragma once

class QVulkanWindow;

struct Buffer
{
    VkBuffer       buffer = nullptr;
    VkDeviceMemory memory = nullptr;
    uint32_t       size   = 0u;
};

struct IVulkanFactory
{
    static std::unique_ptr<IVulkanFactory> Create(const QVulkanWindow& window);

    virtual Buffer CreateBuffer(
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memory_property,
        VkDeviceSize size,
        void* data
    ) = 0;

    virtual ~IVulkanFactory() = default;
};
