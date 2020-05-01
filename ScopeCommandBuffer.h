#pragma once

#include <vulkan/vulkan.h>

class QVulkanWindow;

namespace Vulkan
{
class ScopeCommandBuffer
{
public:
    ScopeCommandBuffer(const QVulkanWindow& window);
    ~ScopeCommandBuffer();

    void TransferBarrier(
        VkPipelineStageFlagBits stage_before,
        VkPipelineStageFlagBits stage_after,
        const VkImageMemoryBarrier& barrier
    ) const;
    void CopyImage(VkImage src, VkImage dst, const VkImageCopy& info) const;

private:
    const QVulkanWindow& m_window;

    VkCommandBuffer command_buffer = nullptr;
};


}
