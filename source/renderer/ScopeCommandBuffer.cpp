#include "ScopeCommandBuffer.h"

#include "Common.h"
#include "Utils.h"

namespace Vulkan
{

VkCommandBuffer CreateCommandBuffer(VulkanShared& vulkan)
{
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = vulkan.command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    VkCommandBuffer cmd_buffer = nullptr;
    VkResultSuccess(vkAllocateCommandBuffers(vulkan.device, &allocate_info, &cmd_buffer));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResultSuccess(vkBeginCommandBuffer(cmd_buffer, &begin_info));
    return cmd_buffer;
}

void FlushCommandBuffer(VkCommandBuffer command_buffer, VulkanShared& vulkan)
{
    if (!command_buffer)
        return;

    VkResultSuccess(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;

    VkResultSuccess(vkCreateFence(vulkan.device, &fence_info, nullptr, &fence));
    VkResultSuccess(vkQueueSubmit(vulkan.graphics_queue, 1, &submit_info, fence));
    VkResultSuccess(vkWaitForFences(vulkan.device, 1, &fence, VK_TRUE, 100000000000));

    vkDestroyFence(vulkan.device, fence, nullptr);
    vkFreeCommandBuffers(vulkan.device, vulkan.command_pool, 1, &command_buffer);
}

ScopeCommandBuffer::ScopeCommandBuffer(VulkanShared& vulkan)
    : vulkan(vulkan)
    , command_buffer(CreateCommandBuffer(vulkan))
{
}

ScopeCommandBuffer::~ScopeCommandBuffer()
{
    FlushCommandBuffer(command_buffer, vulkan);
}

void ScopeCommandBuffer::TransferBarrier(VkPipelineStageFlagBits stage_before, VkPipelineStageFlagBits stage_after, const VkImageMemoryBarrier& barrier) const
{
    vkCmdPipelineBarrier(
        command_buffer,
        stage_before,
        stage_after,
        0, 0, nullptr, 0, nullptr,
        1, &barrier
    );
}

void ScopeCommandBuffer::CopyImage(VkImage src, VkImage dst, const VkImageCopy& info) const
{
   vkCmdCopyImage(
        command_buffer,
        src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &info
    );
}

void ScopeCommandBuffer::CopyBuffer(VkBuffer src, VkBuffer dst, const VkBufferCopy& info) const
{
    vkCmdCopyBuffer(
        command_buffer,
        src,
        dst,
        1,
        &info
    );
}

}