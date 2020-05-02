#pragma once

#include <QVulkanWindow>

#include "IRenderer.h"

namespace Vulkan
{

class Pipeline
    : public IPipeline
{
public:
    Pipeline(const IDescriptorSet&, const Shaders&, const IVertexBuffer&, const QVulkanWindow& window);
    ~Pipeline() override;

    void Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const;

private:
    QVulkanDeviceFunctions& functions;
    VkDevice device = nullptr;

    VkPipeline      pipeline = nullptr;
    VkPipelineCache pipeline_cache = nullptr;
};

}