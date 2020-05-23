#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"

class QVulkanWindow;

namespace Vulkan
{

struct ICamera;
struct CameraRaii;


struct CommandBuffer
    : public ICommandBuffer
{
    CommandBuffer(bool primary, uint32_t queue_node_index, ICamera& camera, const QVulkanWindow& wnd);
    ~CommandBuffer() override;

    void Bind(const IDescriptorSet&) const override;
    void Bind(const IPipeline&) const override;
    void Bind(const IBuffer&) const override;
    void Draw(const IBuffer&, uint32_t count, uint32_t offset) const override;

    void Begin(const VkCommandBufferBeginInfo& begin_info);
    void End();
    void Submit();

    VkCommandBuffer Get() const { return self; }

private:
    const QVulkanWindow& window;
    CameraRaii& camera_raii;

    VkFence fence = nullptr;
    VkCommandBuffer self = nullptr;
    VkCommandPool command_pool = nullptr;

    mutable uint32_t current_index_count = 0;
};

class RenderPass
    : public IRenderPass
{
public:
    RenderPass(CommandBuffer& primary, ICamera& camera, const QVulkanWindow& wnd);
    void AddCommandBuffer(ICommandBuffer&) override;
    ~RenderPass() override;

private:
    const QVulkanWindow& window;
    CameraRaii& camera_raii;
    CommandBuffer& primary;

    VkCommandBufferInheritanceInfo inheritance_info{};
    std::vector<std::reference_wrapper<CommandBuffer>> command_buffers;
};

}