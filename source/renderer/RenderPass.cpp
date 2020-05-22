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

    VkCommandBuffer command_buffer = window.currentCommandBuffer();
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

    device_functions.vkCmdBeginRenderPass(command_buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width = size.width();
    viewport.height = size.height();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    device_functions.vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent.width = size.width();
    scissor.extent.height = size.height();
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    device_functions.vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

RenderPass::~RenderPass()
{
    window.vulkanInstance()->deviceFunctions(window.device())->vkCmdEndRenderPass(window.currentCommandBuffer());
    camera_raii.AfterRender();
}

void RenderPass::Bind(const IDescriptorSet& desc_set) const
{
    auto descriptor_set = dynamic_cast<const DescriptorSet*>(&desc_set);
    if (!descriptor_set)
        throw std::logic_error("Unknown descriptor set derived");

    auto& dev_funcs = *window.vulkanInstance()->deviceFunctions(window.device());
    descriptor_set->Bind(dev_funcs, window.currentCommandBuffer());

    camera_raii.Push(dev_funcs, window.currentCommandBuffer(), descriptor_set->GetPipelineLayout());
}

void RenderPass::Bind(const IPipeline& pip) const
{
    auto pipeline = dynamic_cast<const Pipeline*>(&pip);
    if (!pipeline)
        throw std::logic_error("Unknown pipeline set derived");

    pipeline->Bind(window.currentCommandBuffer());
}

void RenderPass::Bind(const IBuffer& buffer) const
{
    const auto& buffer_impl = dynamic_cast<const Buffer&>(buffer);

    if (buffer_impl.GetUsage() == BufferUsage::Index)
        current_index_count = buffer_impl.GetWidth();

    buffer_impl.Bind(window.currentCommandBuffer());
}

void RenderPass::Draw(const IBuffer& buffer, uint32_t count, uint32_t offset) const
{
    const auto& inst_buffer = dynamic_cast<const Buffer&>(buffer);

    auto& dev_funcs = *window.vulkanInstance()->deviceFunctions(window.device());
    inst_buffer.Bind(window.currentCommandBuffer());

    dev_funcs.vkCmdDrawIndexed(window.currentCommandBuffer(), current_index_count, count > 0 ? count : inst_buffer.GetWidth(), 0, 0, offset);
}

}
