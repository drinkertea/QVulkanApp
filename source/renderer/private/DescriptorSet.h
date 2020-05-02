#pragma once

#include <QVulkanWindow>

#include "IRenderer.h"

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

        VkDescriptorSetLayout m_descriptor_set_layout = nullptr;
        VkDescriptorSet       m_descriptor_set = nullptr;
        VkDescriptorPool      m_descriptor_pool = nullptr;
        VkPipelineLayout      m_pipeline_layout = nullptr;
    };
}