#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"
#include "IFactory.h"

class QVulkanDeviceFunctions;
class QVulkanWindow;

namespace Vulkan
{

class Pipeline
    : public IPipeline
{
public:
    Pipeline(const IDescriptorSet&, const Shaders&, const IVertexBuffer&, const QVulkanWindow& window, const IInstanceBuffer* instance = nullptr);
    Pipeline(const IDescriptorSet&, const Shaders&, const IVertexBuffer&, const IInstanceBuffer& instance, const QVulkanWindow& window);
    ~Pipeline() override;

    void Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const;

private:
    QVulkanDeviceFunctions& functions;
    VkDevice device = nullptr;

    VkPipeline      pipeline = nullptr;
    VkPipelineCache pipeline_cache = nullptr;
};

}