#include "Factory.h"
#include "Texture.h"
#include "VulkanUtils.h"

#include "ScopeCommandBuffer.h"
#include "MappedData.h"

#include <QVulkanFunctions>

namespace Vulkan
{
    VkDeviceMemory Allocate(uint32_t index, VkDeviceSize size, const QVulkanWindow& window)
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);
        VkMemoryAllocateInfo allocate_info = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            nullptr,
            size,
            index
        };

        VkDeviceMemory res = nullptr;
        VkResultSuccess(device_functions.vkAllocateMemory(device, &allocate_info, nullptr, &res));
        return res;
    }

    Image CreateUploadImage(VkFormat format, uint32_t width, uint32_t height, const QVulkanWindow& window)
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);
        Image image{};

        auto upload_info = GetImageCreateInfo(format, width, height);
        upload_info.tiling = VK_IMAGE_TILING_LINEAR;
        upload_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkResultSuccess(device_functions.vkCreateImage(device, &upload_info, nullptr, &image.image));

        VkMemoryRequirements memory_requiments = {};
        device_functions.vkGetImageMemoryRequirements(device, image.image, &memory_requiments);
        image.memory = Allocate(window.hostVisibleMemoryIndex(), memory_requiments.size, window);

        VkResultSuccess(device_functions.vkBindImageMemory(device, image.image, image.memory, 0));

        return image;
    }

    Image Create2dImage(VkFormat format, uint32_t width, uint32_t height, const QVulkanWindow& window)
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);
        Image image{};

        auto upload_info = GetImageCreateInfo(format, width, height);
        upload_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        upload_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VkResultSuccess(device_functions.vkCreateImage(device, &upload_info, nullptr, &image.image));

        VkMemoryRequirements memory_requiments = {};
        device_functions.vkGetImageMemoryRequirements(device, image.image, &memory_requiments);
        image.memory = Allocate(window.deviceLocalMemoryIndex(), memory_requiments.size, window);

        VkResultSuccess(device_functions.vkBindImageMemory(device, image.image, image.memory, 0));

        return image;
    }

    void CopyImage(const Image& src, const Image& dst, const VkExtent3D& extent, const QVulkanWindow& window)
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);
        ScopeCommandBuffer command_bufer(window);

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

    VkSampler CreateSampler(const QVulkanWindow& window)
    {
        VkSampler res = nullptr;
        VkResultSuccess(window.vulkanInstance()->deviceFunctions(window.device())->vkCreateSampler(
            window.device(), &GetSamplerCreateInfo(), nullptr, &res
        ));
        return res;
    }

    VkImageView CreateImageView(VkImage image, VkFormat format, const QVulkanWindow& window)
    {
        VkImageView res = nullptr;
        VkResultSuccess(window.vulkanInstance()->deviceFunctions(window.device())->vkCreateImageView(
            window.device(), &GetImageViewCreateInfo(image, format), nullptr, &res
        ));
        return res;
    }

    Texture::Texture(const IDataProvider& data, const QVulkanWindow& window, VkFormat format)
        : device(window.device())
        , functions(*window.vulkanInstance()->deviceFunctions(window.device()))
        , image(Create2dImage(format, data.GetWidth(), data.GetHeight(), window))
        , sampler(CreateSampler(window))
        , view(CreateImageView(image.image, format, window))
    {

        auto upload_image = CreateUploadImage(format, data.GetWidth(), data.GetHeight(), window);
        {
            MappedData mapped_data(upload_image, window);
            memcpy(mapped_data.GetData(), data.GetData(), data.GetSize());
        }
        VkExtent3D extent = { data.GetWidth(), data.GetHeight(), 1 };
        CopyImage(upload_image, image, extent, window);

        functions.vkDestroyImage(device, upload_image.image, nullptr);
        functions.vkFreeMemory(device, upload_image.memory, nullptr);
    }

    VkDescriptorImageInfo Texture::GetInfo() const
    {
        VkDescriptorImageInfo info{};
        info.sampler     = sampler;
        info.imageView   = view;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    }

    Texture::~Texture()
    {
        functions.vkDestroyImage(device, image.image, nullptr);
        functions.vkFreeMemory(device, image.memory, nullptr);
        functions.vkDestroySampler(device, sampler, nullptr);
        functions.vkDestroyImageView(device, view, nullptr);
    }
}