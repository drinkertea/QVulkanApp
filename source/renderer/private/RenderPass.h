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
    void Bind(const IIndexBuffer&, const IVertexBuffer&) const override;
    void Draw(const IIndexBuffer&, const IVertexBuffer&) const override;
    void Draw(const IInstanceBuffer&) const override;

private:
    const QVulkanWindow& window;
    CameraRaii& camera_raii;

    mutable uint32_t current_index_count = 0;
};

}