#include "DescriptorSet.h"
#include "Texture.h"
#include "Buffer.h"
#include "VulkanUtils.h"

#include <QVulkanFunctions>

namespace Vulkan
{
    DescriptorSet::DescriptorSet(const InputResources& bindings, const QVulkanWindow& window)
        : device(window.device())
        , functions(*window.vulkanInstance()->deviceFunctions(window.device()))
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

        std::vector<VkDescriptorType> binding_types(bindings.size());
        std::vector<VkDescriptorImageInfo> textures(bindings.size());
        std::vector<VkDescriptorBufferInfo> buffers(bindings.size());
        std::vector<VkDescriptorPoolSize> pool_sizes(bindings.size());
        std::vector<VkDescriptorSetLayoutBinding> binding_layouts(bindings.size());
        std::vector<VkWriteDescriptorSet> write_descriptor_sets(bindings.size());

        for (uint32_t i = 0; i < bindings.size(); ++i)
        {
            const auto& binding = bindings[i];
            if (auto texture = dynamic_cast<const Texture*>(&binding.get()))
            {
                binding_types[i] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                textures[i] = texture->GetInfo();
                binding_layouts[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                write_descriptor_sets[i].pImageInfo = &textures[i];

            }
            else if (auto buffer = dynamic_cast<const UniformBuffer*>(&binding.get()))
            {
                binding_types[i] = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                buffers[i] = buffer->GetInfo();
                binding_layouts[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                write_descriptor_sets[i].pBufferInfo = &buffers[i];
            }
            else
            {
                throw std::logic_error("Wrong inout derived class");
            }

            pool_sizes[i].type = binding_types[i];
            pool_sizes[i].descriptorCount = 1;

            binding_layouts[i].descriptorCount = 1;
            binding_layouts[i].descriptorType = binding_types[i];
            binding_layouts[i].binding = i;

            write_descriptor_sets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_sets[i].descriptorCount = 1;
            write_descriptor_sets[i].descriptorType = binding_types[i];
            write_descriptor_sets[i].dstBinding = i;
        }

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes    = pool_sizes.data();
        pool_info.maxSets       = 2;

        Vulkan::VkResultSuccess(device_functions.vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool));

        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
        descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_info.pBindings = binding_layouts.data();
        descriptor_set_layout_info.bindingCount = static_cast<uint32_t>(binding_layouts.size());

        Vulkan::VkResultSuccess(device_functions.vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout));

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &descriptor_set_layout;

        Vulkan::VkResultSuccess(device_functions.vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout));

        VkDescriptorSetAllocateInfo descriptor_set_info{};
        descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_info.descriptorPool     = descriptor_pool;
        descriptor_set_info.pSetLayouts        = &descriptor_set_layout;
        descriptor_set_info.descriptorSetCount = 1;

        Vulkan::VkResultSuccess(device_functions.vkAllocateDescriptorSets(device, &descriptor_set_info, &descriptor_set));

        for (uint32_t i = 0; i < bindings.size(); ++i)
            write_descriptor_sets[i].dstSet = descriptor_set;

        device_functions.vkUpdateDescriptorSets(device, static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
    }

    DescriptorSet::~DescriptorSet()
    {
        functions.vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
        functions.vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        functions.vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    }

    void DescriptorSet::Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf)
    {
        vulkan.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
    }

    VkPipelineLayout DescriptorSet::GetPipelineLayout() const
    {
        return pipeline_layout;
    }
}
