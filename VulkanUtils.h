#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>

namespace Vulkan
{
    struct InvalidVulkanCall : public std::logic_error
    {
        InvalidVulkanCall();
    };

    void VkResultSuccess(VkResult res);

    VkImageCreateInfo     GetImageCreateInfo(VkFormat format, uint32_t width, uint32_t height);
    VkSamplerCreateInfo   GetSamplerCreateInfo();
    VkImageViewCreateInfo GetImageViewCreateInfo(VkImage image, VkFormat format);
}
