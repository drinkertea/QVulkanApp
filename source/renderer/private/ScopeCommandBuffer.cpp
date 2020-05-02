#include <QVulkanWindow>
#include <QVulkanFunctions>

#include "ScopeCommandBuffer.h"
#include "VulkanFactory.h"
#include "VulkanUtils.h"

namespace Vulkan
{

VkCommandBuffer CreateCommandBuffer(const QVulkanWindow& window)
{
    auto& device_functions = *window.vulkanInstance()->deviceFunctions(window.device());

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = window.graphicsCommandPool();
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    VkCommandBuffer cmd_buffer = nullptr;
    VkResultSuccess(device_functions.vkAllocateCommandBuffers(window.device(), &allocate_info, &cmd_buffer));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResultSuccess(device_functions.vkBeginCommandBuffer(cmd_buffer, &begin_info));
    return cmd_buffer;
}

void FlushCommandBuffer(VkCommandBuffer command_buffer, const QVulkanWindow& window)
{
    if (!command_buffer)
        return;

    auto& device_functions = *window.vulkanInstance()->deviceFunctions(window.device());

    VkResultSuccess(device_functions.vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = nullptr;

    VkResultSuccess(device_functions.vkCreateFence(window.device(), &fence_info, nullptr, &fence));
    VkResultSuccess(device_functions.vkQueueSubmit(window.graphicsQueue(), 1, &submit_info, fence));
    VkResultSuccess(device_functions.vkWaitForFences(window.device(), 1, &fence, VK_TRUE, 100000000000));

    device_functions.vkDestroyFence(window.device(), fence, nullptr);
    device_functions.vkFreeCommandBuffers(window.device(), window.graphicsCommandPool(), 1, &command_buffer);
}

ScopeCommandBuffer::ScopeCommandBuffer(const QVulkanWindow& window)
    : m_window(window)
    , command_buffer(CreateCommandBuffer(m_window))
{
}

ScopeCommandBuffer::~ScopeCommandBuffer()
{
    FlushCommandBuffer(command_buffer, m_window);
}

void ScopeCommandBuffer::TransferBarrier(VkPipelineStageFlagBits stage_before, VkPipelineStageFlagBits stage_after, const VkImageMemoryBarrier& barrier) const
{
    m_window.vulkanInstance()->deviceFunctions(m_window.device())->vkCmdPipelineBarrier(
        command_buffer,
        stage_before,
        stage_after,
        0, 0, nullptr, 0, nullptr,
        1, &barrier
    );
}

void ScopeCommandBuffer::CopyImage(VkImage src, VkImage dst, const VkImageCopy& info) const
{
    m_window.vulkanInstance()->deviceFunctions(m_window.device())->vkCmdCopyImage(
        command_buffer,
        src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &info
    );
}

}