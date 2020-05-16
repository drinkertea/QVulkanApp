#include "Utils.h"

#include <QVulkanFunctions>
#include <QVulkanWindow>
#include "Shader.h"
#include "Buffer.h"
#include "DescriptorSet.h"
#include "Pipeline.h"

namespace Vulkan
{

Pipeline::Pipeline(const IDescriptorSet& descriptor_set, const Shaders& shaders, const IVertexBuffer& vertex, const IInstanceBuffer& instance, const QVulkanWindow& window)
    : Pipeline(descriptor_set, shaders, vertex, window, &instance)
{
}

Pipeline::Pipeline(const IDescriptorSet& descriptor_set, const Shaders& shaders, const IVertexBuffer& vertex, const QVulkanWindow& window, const IInstanceBuffer* instance)
    : device(window.device())
    , functions(*window.vulkanInstance()->deviceFunctions(window.device()))
{
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info{};
    input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_info.flags = 0;
    input_assembly_state_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterization_state_info{};
    rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_info.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = 0xf;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_SUBTRACT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_info.attachmentCount = 1;
    color_blend_state_info.pAttachments = &color_blend_attachment;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
   // depth_stencil_state_info.back.compareOp = VK_COMPARE_OP_ALWAYS;

    VkPipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.flags = 0;

    VkPipelineMultisampleStateCreateInfo multisample_state_info{};
    multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_info.flags = 0;

    std::vector<VkDynamicState> dynamic_state_enables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pDynamicStates = dynamic_state_enables.data();
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_state_enables.size());
    dynamic_state_info.flags = 0;

    VkPipelineShaderStageCreateInfo default_shader_info{};
    default_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    default_shader_info.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shader_info(shaders.size(), default_shader_info);
    for (uint32_t i = 0; i < shaders.size(); ++i)
    {
        auto shader = dynamic_cast<const Shader*>(&shaders[i].get());
        if (!shader)
            std::logic_error("Unknown shader derived class");

        shader_info[i].stage = shader->GetStage();
        shader_info[i].module = shader->GetModule();
    }

    auto desc_set = dynamic_cast<const DescriptorSet*>(&descriptor_set);
    if (!desc_set)
        throw std::logic_error("Unknown desc set derived class");

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = desc_set->GetPipelineLayout();
    pipeline_info.renderPass = window.defaultRenderPass();
    pipeline_info.flags = 0;
    pipeline_info.basePipelineIndex = -1;
    pipeline_info.basePipelineHandle = nullptr;

    auto vertex_buffer = dynamic_cast<const VertexBuffer*>(&vertex);
    if (!vertex_buffer)
        throw std::logic_error("Unknown vertex derived class");

    VertexLayout res_layout = vertex_buffer->GetLayout();

    if (instance)
    {
        auto instance_buffer = dynamic_cast<const InstanceBuffer*>(instance);
        if (!instance_buffer)
            throw std::logic_error("Unknown vertex derived class");

        const auto& layout = instance_buffer->GetLayout();
        res_layout.attributes.insert(res_layout.attributes.end(), layout.attributes.begin(), layout.attributes.end());
        res_layout.bindings.insert(res_layout.bindings.end(), layout.bindings.begin(), layout.bindings.end());
    }

    for (uint32_t i = 0; i < res_layout.attributes.size(); ++i)
        res_layout.attributes[i].location = i;

    auto vertex_info = res_layout.GetDesc();
    pipeline_info.pVertexInputState    = &vertex_info;
    pipeline_info.pInputAssemblyState  = &input_assembly_state_info;
    pipeline_info.pRasterizationState  = &rasterization_state_info;
    pipeline_info.pColorBlendState     = &color_blend_state_info;
    pipeline_info.pMultisampleState    = &multisample_state_info;
    pipeline_info.pViewportState       = &viewport_state_info;
    pipeline_info.pDepthStencilState   = &depth_stencil_state_info;
    pipeline_info.pDynamicState        = &dynamic_state_info;
    pipeline_info.stageCount           = static_cast<uint32_t>(shader_info.size());
    pipeline_info.pStages              = shader_info.data();

    auto device = window.device();
    auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    Vulkan::VkResultSuccess(device_functions.vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipeline_cache));
    Vulkan::VkResultSuccess(device_functions.vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_info, nullptr, &pipeline));
}

Pipeline::~Pipeline()
{
    functions.vkDestroyPipeline(device, pipeline, nullptr);
    functions.vkDestroyPipelineCache(device, pipeline_cache, nullptr);
}

void Pipeline::Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const
{
    vulkan.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

}
