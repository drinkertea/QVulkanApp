#include <QVulkanWindow>
#include <QVulkanFunctions>

#include "VulkanFactory.h"
#include "VulkanUtils.h"
#include "MappedData.h"
#include "ScopeCommandBuffer.h"

class VulkanFactory
    : public Vulkan::IVulkanFactory
{
    const QVulkanWindow&    m_window;
    VkDevice                m_device = nullptr;
    QVulkanFunctions&       m_vulkan_functions;
    QVulkanDeviceFunctions& m_device_functions;

    VkPhysicalDeviceMemoryProperties m_memory_properties;

    uint32_t GetMemoryIndex(uint32_t type_bits, VkMemoryPropertyFlags memory_property) const
    {
        for (uint32_t i = 0; i < m_memory_properties.memoryTypeCount; i++)
        {
            if ((type_bits & 1) == 1 && (m_memory_properties.memoryTypes[i].propertyFlags & memory_property) == memory_property)
                return i;
            type_bits >>= 1;
        }
        throw std::runtime_error("Could not find a matching memory type");
    }

    VkDeviceMemory Allocate(uint32_t index, VkDeviceSize size) const
    {
        VkMemoryAllocateInfo allocate_info = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            nullptr,
            size,
            index
        };

        VkDeviceMemory res = nullptr;
        Vulkan::VkResultSuccess(m_device_functions.vkAllocateMemory(m_device, &allocate_info, nullptr, &res));
        return res;
    }

public:
    VulkanFactory(const QVulkanWindow& window)
        : m_window(window)
        , m_device(m_window.device())
        , m_vulkan_functions(*m_window.vulkanInstance()->functions())
        , m_device_functions(*m_window.vulkanInstance()->deviceFunctions(m_device))
    {
        m_vulkan_functions.vkGetPhysicalDeviceMemoryProperties(m_window.physicalDevice(), &m_memory_properties);
    }

    ~VulkanFactory() override = default;

    Vulkan::Buffer CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_property, VkDeviceSize size, void* data) const override
    {
        Vulkan::Buffer buffer = {};
        buffer.size = size;

        VkBufferCreateInfo buffer_createInfo{};
        buffer_createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_createInfo.usage = usage;
        buffer_createInfo.size  = size;

        Vulkan::VkResultSuccess(m_device_functions.vkCreateBuffer(m_device, &buffer_createInfo, nullptr, &buffer.buffer));
        
        VkMemoryRequirements memory_requiments{};
        m_device_functions.vkGetBufferMemoryRequirements(m_device, buffer.buffer, &memory_requiments);

        buffer.memory = Allocate(GetMemoryIndex(memory_requiments.memoryTypeBits, memory_property), memory_requiments.size);

        //buffer->alignment = memory_requiments.alignment;
        //buffer->usageFlags = usageFlags;
        //buffer->memoryPropertyFlags = memoryPropertyFlags;

        if (data)
        {
            void* mapped = nullptr;
            Vulkan::VkResultSuccess(m_device_functions.vkMapMemory(m_device, buffer.memory, 0, size, 0, &mapped));
            memcpy(mapped, data, size);
            if ((memory_property & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange mapped_range = {};
                mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                mapped_range.memory = buffer.memory;
                mapped_range.offset = 0;
                mapped_range.size = size;
                Vulkan::VkResultSuccess(m_device_functions.vkFlushMappedMemoryRanges(m_device, 1, &mapped_range));
            }
            m_device_functions.vkUnmapMemory(m_device, buffer.memory);
        }

        Vulkan::VkResultSuccess(m_device_functions.vkBindBufferMemory(m_device, buffer.buffer, buffer.memory, 0));

        return buffer;
    }

    Vulkan::Image CreateUploadImage(VkFormat format, uint32_t width,uint32_t height) const override
    {
        Vulkan::Image image{};

        auto upload_info = Vulkan::GetImageCreateInfo(format, width, height);
        upload_info.tiling = VK_IMAGE_TILING_LINEAR;
        upload_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        Vulkan::VkResultSuccess(m_device_functions.vkCreateImage(m_device, &upload_info, nullptr, &image.image));

        VkMemoryRequirements memory_requiments = {};
        m_device_functions.vkGetImageMemoryRequirements(m_device, image.image, &memory_requiments);
        image.memory = Allocate(m_window.hostVisibleMemoryIndex(), memory_requiments.size);

        Vulkan::VkResultSuccess(m_device_functions.vkBindImageMemory(m_device, image.image, image.memory, 0));

        return image;
    }

    Vulkan::Image Create2dImage(VkFormat format, uint32_t width, uint32_t height) const override
    {
        Vulkan::Image image{};

        auto upload_info = Vulkan::GetImageCreateInfo(format, width, height);
        upload_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        upload_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        Vulkan::VkResultSuccess(m_device_functions.vkCreateImage(m_device, &upload_info, nullptr, &image.image));

        VkMemoryRequirements memory_requiments = {};
        m_device_functions.vkGetImageMemoryRequirements(m_device, image.image, &memory_requiments);
        image.memory = Allocate(m_window.deviceLocalMemoryIndex(), memory_requiments.size);

        Vulkan::VkResultSuccess(m_device_functions.vkBindImageMemory(m_device, image.image, image.memory, 0));

        return image;
    }

    void CopyImage(const Vulkan::Image& src, const Vulkan::Image& dst, const VkExtent3D& extent) const override
    {
        Vulkan::ScopeCommandBuffer command_bufer(m_window);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = barrier.subresourceRange.layerCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.image = src.image;

        command_bufer.TransferBarrier(VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, barrier);

        barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.image = dst.image;

        command_bufer.TransferBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, barrier);

        VkImageCopy copy_info = {};
        copy_info.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_info.srcSubresource.layerCount = 1;
        copy_info.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_info.dstSubresource.layerCount = 1;
        copy_info.extent = extent;

        command_bufer.CopyImage(src.image, dst.image, copy_info);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.image = dst.image;

        command_bufer.TransferBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, barrier);
    }

    VkSampler CreateSampler() const override
    {
        VkSampler res = nullptr;
        Vulkan::VkResultSuccess(m_window.vulkanInstance()->deviceFunctions(m_window.device())->vkCreateSampler(
            m_window.device(), &Vulkan::GetSamplerCreateInfo(), nullptr, &res
        ));
        return res;
    }

    VkImageView CreateImageView(VkImage image, VkFormat format) const override
    {
        VkImageView res = nullptr;
        Vulkan::VkResultSuccess(m_window.vulkanInstance()->deviceFunctions(m_window.device())->vkCreateImageView(
            m_window.device(), &Vulkan::GetImageViewCreateInfo(image, format), nullptr, &res
        ));
        return res;
    }

    Vulkan::IMappedData::Ptr MapImage(const Vulkan::Image& img) const override
    {
        return std::make_unique<Vulkan::MappedData>(img, m_window);
    }
};

std::unique_ptr<Vulkan::IVulkanFactory> Vulkan::IVulkanFactory::Create(const QVulkanWindow& window)
{
    return std::make_unique<VulkanFactory>(window);
}
