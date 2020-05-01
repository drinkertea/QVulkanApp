#include <QVulkanWindow>
#include <QVulkanFunctions>

#include "VulkanFactory.h"
#include "MappedData.h"
#include "ScopeCommandBuffer.h"

VkImageCreateInfo GetImageCreateInfo(VkFormat format, uint32_t width, uint32_t height)
{
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = format;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    return image_create_info;
}

VkSamplerCreateInfo GetSamplerCreateInfo()
{
    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.mipLodBias = 0.0f;
    sampler.compareOp = VK_COMPARE_OP_NEVER;
    sampler.minLod = 0.0f;
    sampler.maxLod = 0.0f;
    sampler.maxAnisotropy = 1.0;
    sampler.anisotropyEnable = VK_FALSE;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    return sampler;
}

VkImageViewCreateInfo GetImageViewCreateInfo(VkImage image, VkFormat format)
{
    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.baseMipLevel = 0;
    view.subresourceRange.baseArrayLayer = 0;
    view.subresourceRange.layerCount = 1;
    view.subresourceRange.levelCount = 1;
    view.image = image;
    view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    return view;
}

class VulkanFactory
    : public IVulkanFactory
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
        VkResultSuccess(m_device_functions.vkAllocateMemory(m_device, &allocate_info, nullptr, &res));
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

    Buffer CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_property, VkDeviceSize size, void* data) const override
    {
        Buffer buffer = {};
        buffer.size = size;

        VkBufferCreateInfo buffer_createInfo{};
        buffer_createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_createInfo.usage = usage;
        buffer_createInfo.size  = size;

        VkResultSuccess(m_device_functions.vkCreateBuffer(m_device, &buffer_createInfo, nullptr, &buffer.buffer));
        
        VkMemoryRequirements memory_requiments{};
        m_device_functions.vkGetBufferMemoryRequirements(m_device, buffer.buffer, &memory_requiments);

        buffer.memory = Allocate(GetMemoryIndex(memory_requiments.memoryTypeBits, memory_property), memory_requiments.size);

        //buffer->alignment = memory_requiments.alignment;
        //buffer->usageFlags = usageFlags;
        //buffer->memoryPropertyFlags = memoryPropertyFlags;

        if (data)
        {
            void* mapped = nullptr;
            VkResultSuccess(m_device_functions.vkMapMemory(m_device, buffer.memory, 0, size, 0, &mapped));
            memcpy(mapped, data, size);
            if ((memory_property & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange mapped_range = {};
                mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                mapped_range.memory = buffer.memory;
                mapped_range.offset = 0;
                mapped_range.size = size;
                VkResultSuccess(m_device_functions.vkFlushMappedMemoryRanges(m_device, 1, &mapped_range));
            }
            m_device_functions.vkUnmapMemory(m_device, buffer.memory);
        }

        VkResultSuccess(m_device_functions.vkBindBufferMemory(m_device, buffer.buffer, buffer.memory, 0));

        return buffer;
    }

    Vulkan::Image CreateUploadImage(VkFormat format, uint32_t width,uint32_t height) const override
    {
        Vulkan::Image image{};

        auto upload_info = GetImageCreateInfo(format, width, height);
        upload_info.tiling = VK_IMAGE_TILING_LINEAR;
        upload_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkResultSuccess(m_device_functions.vkCreateImage(m_device, &upload_info, nullptr, &image.image));

        VkMemoryRequirements memory_requiments = {};
        m_device_functions.vkGetImageMemoryRequirements(m_device, image.image, &memory_requiments);
        image.memory = Allocate(m_window.hostVisibleMemoryIndex(), memory_requiments.size);

        VkResultSuccess(m_device_functions.vkBindImageMemory(m_device, image.image, image.memory, 0));

        return image;
    }

    Vulkan::Image Create2dImage(VkFormat format, uint32_t width, uint32_t height) const override
    {
        Vulkan::Image image{};

        auto upload_info = GetImageCreateInfo(format, width, height);
        upload_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        upload_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VkResultSuccess(m_device_functions.vkCreateImage(m_device, &upload_info, nullptr, &image.image));

        VkMemoryRequirements memory_requiments = {};
        m_device_functions.vkGetImageMemoryRequirements(m_device, image.image, &memory_requiments);
        image.memory = Allocate(m_window.deviceLocalMemoryIndex(), memory_requiments.size);

        VkResultSuccess(m_device_functions.vkBindImageMemory(m_device, image.image, image.memory, 0));

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
        VkResultSuccess(m_window.vulkanInstance()->deviceFunctions(m_window.device())->vkCreateSampler(
            m_window.device(), &GetSamplerCreateInfo(), nullptr, &res
        ));
        return res;
    }

    VkImageView CreateImageView(VkImage image, VkFormat format) const override
    {
        VkImageView res = nullptr;
        VkResultSuccess(m_window.vulkanInstance()->deviceFunctions(m_window.device())->vkCreateImageView(
            m_window.device(), &GetImageViewCreateInfo(image, format), nullptr, &res
        ));
        return res;
    }

    IMappedData::Ptr MapImage(const Vulkan::Image& img) const override
    {
        return std::make_unique<Vulkan::MappedData>(img, m_window);
    }
};

std::unique_ptr<IVulkanFactory> IVulkanFactory::Create(const QVulkanWindow& window)
{
    return std::make_unique<VulkanFactory>(window);
}
