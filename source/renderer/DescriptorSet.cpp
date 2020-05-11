#include "DescriptorSet.h"
#include "Texture.h"
#include "Buffer.h"
#include "Utils.h"
#include "Camera.h"

#include <QVulkanFunctions>
#include <QVulkanWindow>

namespace Vulkan
{
    DescriptorSet::DescriptorSet(const InputResources& bindings, const QVulkanWindow& window)
        : device(window.device())
        , functions(*window.vulkanInstance()->deviceFunctions(window.device()))
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

        std::vector<VkDescriptorType> binding_types;
        std::vector<VkDescriptorImageInfo> textures;
        std::vector<VkDescriptorBufferInfo> buffers;
        std::vector<VkDescriptorPoolSize> pool_sizes;
        std::vector<VkDescriptorSetLayoutBinding> binding_layouts;
        std::vector<VkWriteDescriptorSet> write_descriptor_sets;

        textures.reserve(bindings.size());
        buffers.reserve(bindings.size());

        std::vector<VkPushConstantRange> push_constant_ranges{};

        for (const auto& binding : bindings)
        {
            VkDescriptorSetLayoutBinding layout{};
            VkWriteDescriptorSet write_descriptor_set{};
            if (auto texture = dynamic_cast<const Texture*>(&binding.get()))
            {
                binding_types.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                textures.push_back(texture->GetInfo());
                layout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                write_descriptor_set.pImageInfo = &textures.back();
            }
            else if (auto buffer = dynamic_cast<const UniformBuffer*>(&binding.get()))
            {
                binding_types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                buffers.push_back(buffer->GetInfo());
                layout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                write_descriptor_set.pBufferInfo = &buffers.back();
            }
            else if (auto pc = dynamic_cast<const PushConstantLayout*>(&binding.get()))
            {
                VkPushConstantRange push_constant_range{};
                push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                push_constant_range.offset = pc->GetOffset();
                push_constant_range.size   = pc->GetSize();
                push_constant_ranges.push_back(push_constant_range);
                continue;
            }
            else
            {
                throw std::logic_error("Wrong inout derived class");
            }

            VkDescriptorPoolSize pool_size{};
            pool_size.type = binding_types.back();
            pool_size.descriptorCount = 1;
            pool_sizes.push_back(pool_size);

            layout.descriptorCount = 1;
            layout.descriptorType = binding_types.back();
            layout.binding = static_cast<uint32_t>(binding_layouts.size());
            binding_layouts.push_back(layout);

            write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = binding_types.back();
            write_descriptor_set.dstBinding = static_cast<uint32_t>(write_descriptor_sets.size());
            write_descriptor_sets.push_back(write_descriptor_set);
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
        pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
        pipeline_layout_info.pPushConstantRanges = push_constant_ranges.data();

        Vulkan::VkResultSuccess(device_functions.vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout));

        VkDescriptorSetAllocateInfo descriptor_set_info{};
        descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_info.descriptorPool     = descriptor_pool;
        descriptor_set_info.pSetLayouts        = &descriptor_set_layout;
        descriptor_set_info.descriptorSetCount = 1;

        Vulkan::VkResultSuccess(device_functions.vkAllocateDescriptorSets(device, &descriptor_set_info, &descriptor_set));

        for (auto& wds : write_descriptor_sets)
            wds.dstSet = descriptor_set;

        device_functions.vkUpdateDescriptorSets(device, static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
    }

    DescriptorSet::~DescriptorSet()
    {
        functions.vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
        functions.vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        functions.vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    }

    void DescriptorSet::Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const
    {
        vulkan.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
    }

    VkPipelineLayout DescriptorSet::GetPipelineLayout() const
    {
        return pipeline_layout;
    }
}
