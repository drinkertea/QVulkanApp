#pragma once

#include <vulkan/vulkan.h>
#include "IFactory.h"

namespace Vulkan
{

class DescriptorSet;
struct VertexLayout;
struct VulkanShared;

class Pipeline
    : public IPipeline
{
public:
    Pipeline(const DescriptorSet&, const Shaders&, const VertexLayout&, VulkanShared& vulkan);
    ~Pipeline() override;

    void Bind(VkCommandBuffer cmd_buf) const;

private:
    VulkanShared& vulkan;

    VkPipeline      pipeline = nullptr;
    VkPipelineCache pipeline_cache = nullptr;
};

}