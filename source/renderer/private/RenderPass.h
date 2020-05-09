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

    void Bind(const IDescriptorSet&, const void*, uint32_t) const override;
    void Bind(const IPipeline&) const override;
    void Bind(const IIndexBuffer&, const IVertexBuffer&) const override;
    void Draw(const IIndexBuffer&, const IVertexBuffer&) const override;
    void Draw(const IInstanceBuffer&) const override;

private:
    const QVulkanWindow& window;

    mutable uint32_t current_index_count = 0;
};

}