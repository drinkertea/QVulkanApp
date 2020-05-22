#pragma once

#include <vulkan/vulkan.h>

#include "IRenderer.h"

namespace Vulkan
{

struct VulkanShared;

class ScopeCommandBuffer
{
public:
    ScopeCommandBuffer(VulkanShared& vulkan);
    ~ScopeCommandBuffer();

    void TransferBarrier(
        VkPipelineStageFlagBits stage_before,
        VkPipelineStageFlagBits stage_after,
        const VkImageMemoryBarrier& barrier
    ) const;
    void CopyImage(VkImage src, VkImage dst, const VkImageCopy& info) const;
    void CopyBuffer(VkBuffer src, VkBuffer dst, const VkBufferCopy& info) const;

private:
    VulkanShared& vulkan;

    VkCommandBuffer command_buffer = nullptr;
};

}
