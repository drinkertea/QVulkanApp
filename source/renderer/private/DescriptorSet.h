#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"
#include "IFactory.h"

class QVulkanDeviceFunctions;
class QVulkanWindow;

namespace Vulkan
{
    class DescriptorSet
        : public IDescriptorSet
    {
    public:
        DescriptorSet(const InputResources&, const QVulkanWindow&);
        ~DescriptorSet() override;

        void Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf);

        VkPipelineLayout GetPipelineLayout() const;

    private:
        QVulkanDeviceFunctions& functions;
        VkDevice device = nullptr;

        VkDescriptorSetLayout descriptor_set_layout = nullptr;
        VkDescriptorSet       descriptor_set = nullptr;
        VkDescriptorPool      descriptor_pool = nullptr;
        VkPipelineLayout      pipeline_layout = nullptr;
    };
}