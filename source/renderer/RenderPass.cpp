#include <QVulkanFunctions>
#include <QVulkanWindow>

#include "RenderPass.h"
#include "Utils.h"
#include "Buffer.h"
#include "DescriptorSet.h"
#include "Pipeline.h"
#include "Camera.h"

namespace Vulkan
{

CameraRaii& GetCam(ICamera& camera)
{
    auto camera_raii = dynamic_cast<CameraRaii*>(&camera);
    if (!camera_raii)
        throw std::logic_error("Wrong camerra impl");

    return *camera_raii;
}

RenderPass::RenderPass(ICamera& camera, const QVulkanWindow& wnd)
    : window(wnd)
    , camera_raii(GetCam(camera))
{
    camera_raii.BeforeRender();
    auto device = window.device();
    auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

    const QSize size = window.swapChainImageSize();

    VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };

    VkCommandBufferBeginInfo command_buffer_info{};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkClearValue clearValues[2];
    clearValues[0].color = defaultClearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.framebuffer = window.currentFramebuffer();
    renderPassBeginInfo.renderPass = window.defaultRenderPass();
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = size.width();
    renderPassBeginInfo.renderArea.extent.height = size.height();
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance_info.framebuffer = window.currentFramebuffer();
    inheritance_info.renderPass = window.defaultRenderPass();

    device_functions.vkCmdBeginRenderPass(window.currentCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void RenderPass::AddCommandBuffer(ICommandBuffer& cmd)
{
    command_buffers.emplace_back(dynamic_cast<CommandBuffer&>(cmd));

    VkCommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmd_begin_info.pInheritanceInfo = &inheritance_info;

    command_buffers.back().get().Begin(cmd_begin_info);
}

RenderPass::~RenderPass()
{
    auto device = window.device();
    auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

    std::vector<VkCommandBuffer> to_execute;
    to_execute.reserve(command_buffers.size());

    for (auto& cmd : command_buffers)
    {
        command_buffers.back().get().End();
        to_execute.emplace_back(command_buffers.back().get().Get());
    }

    if (!to_execute.empty())
        device_functions.vkCmdExecuteCommands(
            window.currentCommandBuffer(),
            static_cast<uint32_t>(to_execute.size()),
            to_execute.data()
        );

    device_functions.vkCmdEndRenderPass(window.currentCommandBuffer());

    camera_raii.AfterRender();
}

CommandBuffer::CommandBuffer(uint32_t queue_node_index, ICamera& camera, const QVulkanWindow& wnd)
    : window(wnd)
    , camera_raii(GetCam(camera))
{
    auto device = window.device();
    auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

    VkCommandPoolCreateInfo cmd_pool_info{};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = queue_node_index;
    VkResultSuccess(device_functions.vkCreateCommandPool(device, &cmd_pool_info, nullptr, &command_pool));

    VkCommandBufferAllocateInfo cmd_buff_allocate_info{};
    cmd_buff_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buff_allocate_info.commandPool = command_pool;
    cmd_buff_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    cmd_buff_allocate_info.commandBufferCount = window.concurrentFrameCount();

    self_instances.resize(window.concurrentFrameCount());
    VkResultSuccess(device_functions.vkAllocateCommandBuffers(device, &cmd_buff_allocate_info, self_instances.data()));
}

CommandBuffer::~CommandBuffer()
{
    auto device = window.device();
    auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);
    device_functions.vkFreeCommandBuffers(device, command_pool, self_instances.size(), self_instances.data());
    device_functions.vkDestroyCommandPool(device, command_pool, nullptr);
}

void CommandBuffer::Bind(const IDescriptorSet& desc_set) const
{
    auto descriptor_set = dynamic_cast<const DescriptorSet*>(&desc_set);
    if (!descriptor_set)
        throw std::logic_error("Unknown descriptor set derived");

    auto& dev_funcs = *window.vulkanInstance()->deviceFunctions(window.device());
    descriptor_set->Bind(dev_funcs, self_instances.at(window.currentFrame()));

    camera_raii.Push(dev_funcs, self_instances.at(window.currentFrame()), descriptor_set->GetPipelineLayout());
}

void CommandBuffer::Bind(const IPipeline& pip) const
{
    auto pipeline = dynamic_cast<const Pipeline*>(&pip);
    if (!pipeline)
        throw std::logic_error("Unknown pipeline set derived");

    pipeline->Bind(self_instances.at(window.currentFrame()));
}

void CommandBuffer::Bind(const IBuffer& buffer) const
{
    const auto& buffer_impl = dynamic_cast<const Buffer&>(buffer);

    if (buffer_impl.GetUsage() == BufferUsage::Index)
        current_index_count = buffer_impl.GetWidth();

    buffer_impl.Bind(self_instances.at(window.currentFrame()));
}

void CommandBuffer::Draw(const IBuffer& buffer, uint32_t count, uint32_t offset) const
{
    const auto& inst_buffer = dynamic_cast<const Buffer&>(buffer);

    auto& dev_funcs = *window.vulkanInstance()->deviceFunctions(window.device());
    inst_buffer.Bind(self_instances.at(window.currentFrame()));

    dev_funcs.vkCmdDrawIndexed(self_instances.at(window.currentFrame()), current_index_count, count > 0 ? count : inst_buffer.GetWidth(), 0, 0, offset);
}

void CommandBuffer::Begin(const VkCommandBufferBeginInfo& begin_info)
{
    auto device = window.device();
    auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

    VkResultSuccess(device_functions.vkBeginCommandBuffer(self_instances.at(window.currentFrame()), &begin_info));

    const QSize size = window.swapChainImageSize();

    VkViewport viewport{};
    viewport.width = size.width();
    viewport.height = size.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    device_functions.vkCmdSetViewport(self_instances.at(window.currentFrame()), 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent.width = size.width();
    scissor.extent.height = size.height();
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    device_functions.vkCmdSetScissor(self_instances.at(window.currentFrame()), 0, 1, &scissor);
}

void CommandBuffer::End()
{
    auto device = window.device();
    auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

    VkResultSuccess(device_functions.vkEndCommandBuffer(self_instances.at(window.currentFrame())));
}

VkCommandBuffer CommandBuffer::Get() const
{
    return self_instances.at(window.currentFrame());
}

}
