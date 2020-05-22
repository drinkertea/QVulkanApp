#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan
{

struct VulkanShared
{
    VkDevice         device          = nullptr;
    VkPhysicalDevice physical_device = nullptr;
    VkCommandPool    command_pool    = nullptr;
    VkQueue          graphics_queue  = nullptr;
    VkRenderPass     render_pass     = nullptr;

    uint32_t host_memory_index   = 0u;
    uint32_t device_memory_index = 0u;
};

}
