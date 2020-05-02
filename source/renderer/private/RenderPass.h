#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"

class QVulkanWindow;

namespace Vulkan
{

class RenderPass
    : public IRenderPass
{
public:
    RenderPass(const QVulkanWindow& wnd);
    ~RenderPass() override;

    void Bind(const IDescriptorSet&) const override;
    void Bind(const IPipeline&) const override;
    void Draw(const IIndexBuffer&, const IVertexBuffer&) const override;

private:
    const QVulkanWindow& window;
};

}