#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"

class QVulkanWindow;

namespace Vulkan
{

struct ICamera;
struct CameraRaii;

class RenderPass
    : public IRenderPass
{
public:
    RenderPass(ICamera& camera, const QVulkanWindow& wnd);
    ~RenderPass() override;

    void Bind(const IDescriptorSet&) const override;
    void Bind(const IPipeline&) const override;
    void Bind(const IBuffer&) const override;
    void Draw(const IBuffer&, uint32_t count, uint32_t offset) const override;

private:
    const QVulkanWindow& window;
    CameraRaii& camera_raii;

    mutable uint32_t current_index_count = 0;
};

}