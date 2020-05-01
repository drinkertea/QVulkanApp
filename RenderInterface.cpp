#include <QVulkanFunctions>
#include <QFile>
#include <QVector2D>

#include <array>
#include <iostream>
#include <sstream>

#include "RenderInterface.h"
#include "VulkanFactory.h"
#include "VulkanUtils.h"
#include "Camera.h"
#include "VulkanImage.h"

struct IvalidPath : public std::runtime_error
{
    IvalidPath(const std::string& msg)
        : std::runtime_error("Cannot find the path specified: " + msg)
    {
    }
};

struct IFrameCallback
{
    virtual void OnFrame(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) = 0;
    virtual ~IFrameCallback() = default;
};

class TextureSource
    : public Vulkan::ITextureSource
{
    QImage image;

public:
    TextureSource(const QString& url)
        : image(url)
    {
        if (image.isNull())
            throw IvalidPath(url.toStdString());

        image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    }

    ~TextureSource() override = default;

    void FillMappedData(const Vulkan::IMappedData& data) const override
    {
        auto mapped_data = data.GetData();
        for (int y = 0; y < image.height(); ++y)
        {
            auto row_pitch = image.width() * 4; // todo: provide format or row pith
            memcpy(mapped_data, image.constScanLine(y), row_pitch);
            mapped_data += row_pitch;
        }
    }

    uint32_t GetWidth() const override
    {
        return image.width();
    }

    uint32_t GetHeight() const override
    {
        return image.height();
    }
};

class IndexedBuffer
    : public IFrameCallback
{
    struct Vertex {
        float pos[3];
        float uv[2];
        float normal[3];
    };

    struct
    {
        VkPipelineVertexInputStateCreateInfo GetDesc() const
        {
            VkPipelineVertexInputStateCreateInfo desc{};
            desc.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            desc.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
            desc.pVertexBindingDescriptions      = bindings.data();
            desc.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
            desc.pVertexAttributeDescriptions    = attributes.data();
            return desc;
        }

        std::vector<VkVertexInputBindingDescription>   bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
    } vertex_layout;

public:
    IndexedBuffer(Vulkan::IVulkanFactory& vulkan_factory)
    {
        std::vector<Vertex> vertices =
        {
            { {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
            { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
            { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
            { {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
        };

        std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
        m_index_count = static_cast<uint32_t>(indices.size());

        m_vectex_buffer = vulkan_factory.CreateBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertices.size() * sizeof(Vertex),
            vertices.data()
        );

        m_index_buffer = vulkan_factory.CreateBuffer(
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            indices.size() * sizeof(uint32_t),
            indices.data()
        );

        VkVertexInputBindingDescription binding_desc{};
        binding_desc.binding   = binding_index;
        binding_desc.stride    = sizeof(Vertex);
        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertex_layout.bindings.emplace_back(binding_desc);

        VkVertexInputAttributeDescription attribute_desc{};
        attribute_desc.binding  = binding_index;

        attribute_desc.location = 0;
        attribute_desc.format   = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_desc.offset   = offsetof(Vertex, pos);
        vertex_layout.attributes.emplace_back(attribute_desc);

        attribute_desc.location = 1;
        attribute_desc.format = VK_FORMAT_R32G32_SFLOAT;
        attribute_desc.offset = offsetof(Vertex, uv);
        vertex_layout.attributes.emplace_back(attribute_desc);

        attribute_desc.location = 2;
        attribute_desc.format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_desc.offset = offsetof(Vertex, normal);
        vertex_layout.attributes.emplace_back(attribute_desc);
    }

    void OnFrame(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) override
    {
        VkDeviceSize offsets[1] = { 0 };
        vulkan.vkCmdBindVertexBuffers(cmd_buf, binding_index, 1, &m_vectex_buffer.buffer, offsets);
        vulkan.vkCmdBindIndexBuffer(cmd_buf, m_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vulkan.vkCmdDrawIndexed(cmd_buf, m_index_count, 1, 0, 0, 0);
    }

    VkPipelineVertexInputStateCreateInfo GetVertexLayoutInfo() const
    {
        return vertex_layout.GetDesc();
    }

    ~IndexedBuffer()
    {
        // TODO
    }

    static constexpr uint32_t binding_index = 0;

private:
    uint32_t m_index_count = 0;
    Vulkan::Buffer m_index_buffer{};
    Vulkan::Buffer m_vectex_buffer{};
};

class UniformBuffer
{
    struct {
        QMatrix4x4 projection;
        QMatrix4x4 model_view;
        QVector4D  view_pos;
    } m_ubo;

public:
    UniformBuffer(Vulkan::IVulkanFactory& vulkan_factory, const QVulkanWindow& window)
    {
        m_buffer = vulkan_factory.CreateBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(m_ubo),
            &m_ubo
        );
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);
        Vulkan::VkResultSuccess(device_functions.vkMapMemory(device, m_buffer.memory, 0, m_buffer.size, 0, reinterpret_cast<void**>(&mapped)));
    }

    void Update(const Camera& camera)
    {
        if (!mapped)
            return;
        constexpr size_t mat_size = 16 * sizeof(float);
        constexpr size_t vec_size = 4 * sizeof(float);

        m_ubo.projection = camera.matrices.perspective;
        m_ubo.model_view = camera.matrices.view;
        m_ubo.view_pos = camera.viewPos;

        float t[4] = { m_ubo.view_pos[0], m_ubo.view_pos[1], m_ubo.view_pos[2], m_ubo.view_pos[3] };
        memcpy(mapped, m_ubo.projection.constData(), mat_size);
        memcpy(mapped + mat_size, m_ubo.model_view.constData(), mat_size);
        memcpy(mapped + 2 * mat_size, t, sizeof(t));

    }

    VkDescriptorBufferInfo GetInfo() const
    {
        VkDescriptorBufferInfo info{};
        info.buffer = m_buffer.buffer;
        info.offset = 0;
        info.range  = VK_WHOLE_SIZE;
        return info;
    }
    ~UniformBuffer()
    {
        // TODO
    }
private:
    uint8_t* mapped = nullptr;

    Vulkan::Buffer m_buffer{};
};

class DescriptorSet
    : public IFrameCallback
{
    VkDescriptorSetLayout m_descriptor_set_layout = nullptr;
    VkDescriptorSet       m_descriptor_set  = nullptr;
    VkDescriptorPool      m_descriptor_pool = nullptr;
    VkPipelineLayout      m_pipeline_layout = nullptr;
public:
    DescriptorSet(const Vulkan::Texture& texture, const UniformBuffer& uniform_buffer, const QVulkanWindow& window)
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

        std::vector<VkDescriptorPoolSize> pool_sizes;

        VkDescriptorPoolSize pool_size{};
        pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_size.descriptorCount = 1;
        pool_sizes.emplace_back(pool_size);

        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size.descriptorCount = 1;
        pool_sizes.emplace_back(pool_size);

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes    = pool_sizes.data();
        pool_info.maxSets       = 2;

        Vulkan::VkResultSuccess(device_functions.vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptor_pool));

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

        VkDescriptorSetLayoutBinding binding_layout{};
        binding_layout.descriptorCount = 1;

        binding_layout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding_layout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        binding_layout.binding = 0;
        setLayoutBindings.emplace_back(binding_layout);

        binding_layout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding_layout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding_layout.binding = 1;
        setLayoutBindings.emplace_back(binding_layout);

        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
        descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_info.pBindings = setLayoutBindings.data();
        descriptor_set_layout_info.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

        Vulkan::VkResultSuccess(device_functions.vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, nullptr, &m_descriptor_set_layout));

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &m_descriptor_set_layout;

        Vulkan::VkResultSuccess(device_functions.vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &m_pipeline_layout));

        VkDescriptorSetAllocateInfo descriptor_set_info{};
        descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_info.descriptorPool     = m_descriptor_pool;
        descriptor_set_info.pSetLayouts        = &m_descriptor_set_layout;
        descriptor_set_info.descriptorSetCount = 1;

        Vulkan::VkResultSuccess(device_functions.vkAllocateDescriptorSets(device, &descriptor_set_info, &m_descriptor_set));

        auto texture_descriptor = texture.GetInfo();
        auto buffer_descriptor  = uniform_buffer.GetInfo();

        std::vector<VkWriteDescriptorSet> write_descriptor_sets;
        VkWriteDescriptorSet write_descriptor_set{};
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.dstSet = m_descriptor_set;

        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.pBufferInfo = &buffer_descriptor;
        write_descriptor_sets.emplace_back(write_descriptor_set);

        write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_set.dstBinding = 1;
        write_descriptor_set.pImageInfo = &texture_descriptor;
        write_descriptor_sets.emplace_back(write_descriptor_set);

        device_functions.vkUpdateDescriptorSets(device, static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);
    }

    void OnFrame(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) override
    {
        vulkan.vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, NULL);
    }

    VkPipelineLayout GetPipelineLayout() const
    {
        return m_pipeline_layout;
    }

    ~DescriptorSet()
    {
        // TODO
    }
};

class Pipeline
    : public IFrameCallback
{
    static VkShaderModule CreateShader(const QString& url, const QVulkanWindow& window)
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

        QFile file(url);
        if (!file.open(QIODevice::ReadOnly))
            throw IvalidPath(url.toStdString());

        QByteArray blob = file.readAll();
        file.close();

        VkShaderModuleCreateInfo shader_info{};
        shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_info.codeSize = blob.size();
        shader_info.pCode = reinterpret_cast<const uint32_t*>(blob.constData());

        VkShaderModule res = nullptr;
        Vulkan::VkResultSuccess(device_functions.vkCreateShaderModule(device, &shader_info, nullptr, &res));
        return res;
    }

public:
    Pipeline(const IndexedBuffer& vertex, const DescriptorSet& descriptor_set, const QVulkanWindow& window)
    {
        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info{};
        input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_state_info.flags = 0;
        input_assembly_state_info.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterization_state_info{};
        rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_info.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state_info.flags = 0;
        rasterization_state_info.depthClampEnable = VK_FALSE;
        rasterization_state_info.lineWidth = 1.0f;

        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = 0xf;
        color_blend_attachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo color_blend_state_info{};
        color_blend_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state_info.attachmentCount = 1;
        color_blend_state_info.pAttachments = &color_blend_attachment;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info{};
        depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state_info.depthTestEnable = VK_TRUE;
        depth_stencil_state_info.depthWriteEnable = VK_TRUE;
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depth_stencil_state_info.back.compareOp = VK_COMPARE_OP_ALWAYS;

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

        VkShaderModule vert_shader = CreateShader(":/texture.vert.spv", window);
        VkShaderModule frag_shader = CreateShader(":/texture.frag.spv", window);
        m_shaders = { vert_shader, frag_shader };

        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
            VkPipelineShaderStageCreateInfo {
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr,
                0,
                VK_SHADER_STAGE_VERTEX_BIT,
                vert_shader,
                "main",
                nullptr
            },
            VkPipelineShaderStageCreateInfo {
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr,
                0,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                frag_shader,
                "main",
                nullptr
            }
        };

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.layout = descriptor_set.GetPipelineLayout();
        pipeline_info.renderPass = window.defaultRenderPass();
        pipeline_info.flags = 0;
        pipeline_info.basePipelineIndex = -1;
        pipeline_info.basePipelineHandle = nullptr;

        auto vertex_info = vertex.GetVertexLayoutInfo();
        pipeline_info.pVertexInputState    = &vertex_info;
        pipeline_info.pInputAssemblyState  = &input_assembly_state_info;
        pipeline_info.pRasterizationState  = &rasterization_state_info;
        pipeline_info.pColorBlendState     = &color_blend_state_info;
        pipeline_info.pMultisampleState    = &multisample_state_info;
        pipeline_info.pViewportState       = &viewport_state_info;
        pipeline_info.pDepthStencilState   = &depth_stencil_state_info;
        pipeline_info.pDynamicState        = &dynamic_state_info;
        pipeline_info.stageCount           = static_cast<uint32_t>(shader_stages.size());
        pipeline_info.pStages              = shader_stages.data();

        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        Vulkan::VkResultSuccess(device_functions.vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_pipeline_cache));
        Vulkan::VkResultSuccess(device_functions.vkCreateGraphicsPipelines(device, m_pipeline_cache, 1, &pipeline_info, nullptr, &m_pipeline));
    }

    void OnFrame(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) override
    {
        vulkan.vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }

private:
    VkPipeline m_pipeline = nullptr;
    VkPipelineCache m_pipeline_cache = nullptr;

    std::vector<VkShaderModule> m_shaders;
};

class VulkanRenderer
    : public IVulkanRenderer
{
    std::unique_ptr<Vulkan::Texture> m_texture;
    std::unique_ptr<IndexedBuffer> m_geometry;
    std::unique_ptr<UniformBuffer> m_uniform;
    std::unique_ptr<DescriptorSet> m_descriptor_set;
    std::unique_ptr<Pipeline>      m_pipeline;

    Camera camera;
    QVector2D mouse_pos;
    bool view_updated = false;
    uint32_t frame_counter = 0;
    float frame_timer = 1.0f;
    float timer = 0.0f;
    float timer_speed = 0.25f;
    bool paused = false;
    uint32_t lastFPS = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp;

public:
    VulkanRenderer(QVulkanWindow& window)
        : m_window(window)
    {
        camera.type = Camera::CameraType::firstperson;
        camera.setPosition(QVector3D(0.0f, 0.0f, -2.5f));
        camera.setRotation(QVector3D(0.0f, 15.0f, 0.0f));
        camera.setPerspective(60.0f, (float)window.width() / (float)window.height(), 0.1f, 256.0f);
    }

    void initResources() override
    {
        m_vulkan_factory = Vulkan::IVulkanFactory::Create(m_window);
        m_texture = std::make_unique<Vulkan::Texture>(*m_vulkan_factory, TextureSource(":/dirt.png"), VK_FORMAT_R8G8B8A8_UNORM);
        m_geometry = std::make_unique<IndexedBuffer>(*m_vulkan_factory);
        m_uniform = std::make_unique<UniformBuffer>(*m_vulkan_factory, m_window);
        m_descriptor_set = std::make_unique<DescriptorSet>(*m_texture, *m_uniform, m_window);
        m_pipeline = std::make_unique<Pipeline>(*m_geometry, *m_descriptor_set, m_window);

        m_uniform->Update(camera);
    }

    void OnKeyPressed(Qt::Key key) override
    {
        if (!camera.firstperson)
            return;

        switch (key)
        {
        case Qt::Key::Key_W:
            camera.keys.up = true;
            break;
        case Qt::Key::Key_S:
            camera.keys.down = true;
            break;
        case Qt::Key::Key_A:
            camera.keys.left = true;
            break;
        case Qt::Key::Key_D:
            camera.keys.right = true;
            break;
        }
    }

    void OnKeyReleased(Qt::Key key) override
    {
        if (!camera.firstperson)
            return;

        switch (key)
        {
        case Qt::Key::Key_W:
            camera.keys.up = false;
            break;
        case Qt::Key::Key_S:
            camera.keys.down = false;
            break;
        case Qt::Key::Key_A:
            camera.keys.left = false;
            break;
        case Qt::Key::Key_D:
            camera.keys.right = false;
            break;
        }
    }

    void OnMouseMove(int32_t x, int32_t y, const Qt::MouseButtons& buttons) override
    {
        int32_t dx = static_cast<int32_t>(mouse_pos.x()) - x;
        int32_t dy = static_cast<int32_t>(mouse_pos.y()) - y;

        if (buttons & Qt::MouseButton::LeftButton)
        {
            camera.rotate(QVector3D(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
            view_updated = true;
        }
        if (buttons & Qt::MouseButton::RightButton)
        {
            camera.translate(QVector3D(-0.0f, 0.0f, dy * .005f));
            view_updated = true;
        }
        if (buttons & Qt::MouseButton::MiddleButton)
        {
            camera.translate(QVector3D(-dx * 0.01f, -dy * 0.01f, 0.0f));
            view_updated = true;
        }
        mouse_pos.setX(x);
        mouse_pos.setY(y);
    }

    void Render()
    {
        auto device = m_window.device();
        auto& device_functions = *m_window.vulkanInstance()->deviceFunctions(device);

        VkCommandBuffer command_buffer = m_window.currentCommandBuffer();
        const QSize size = m_window.swapChainImageSize();

        VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };

        VkCommandBufferBeginInfo command_buffer_info{};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VkClearValue clearValues[2];
        clearValues[0].color = defaultClearColor;
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.framebuffer = m_window.currentFramebuffer();
        renderPassBeginInfo.renderPass = m_window.defaultRenderPass();
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = size.width();
        renderPassBeginInfo.renderArea.extent.height = size.height();
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        device_functions.vkCmdBeginRenderPass(command_buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = size.width();
        viewport.height = size.height();
        viewport.minDepth = 0;
        viewport.maxDepth = 0;
        device_functions.vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent.width = size.width();
        scissor.extent.height = size.height();
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        device_functions.vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        m_descriptor_set->OnFrame(device_functions, command_buffer);
        m_pipeline->OnFrame(device_functions, command_buffer);
        m_geometry->OnFrame(device_functions, command_buffer);

        device_functions.vkCmdEndRenderPass(command_buffer);

        m_window.frameReady();
        m_window.requestUpdate();
    }

    std::string GetWindowTitle()
    {
        std::string device(m_window.physicalDeviceProperties()->deviceName);
        std::string windowTitle;
        windowTitle = "QVulkanApp - " + device;
        windowTitle += " - " + std::to_string(frame_counter) + " fps";
        return windowTitle;
    }


    void startNextFrame() override
    {
        auto render_start = std::chrono::high_resolution_clock::now();
        if (view_updated)
        {
            view_updated = false;
            m_uniform->Update(camera);
        }

        Render();
        frame_counter++;
        auto render_end = std::chrono::high_resolution_clock::now();
        auto time_diff = std::chrono::duration<double, std::milli>(render_end - render_start).count();
        frame_timer = static_cast<float>(time_diff) / 1000.0f;
        camera.update(frame_timer);
        if (camera.moving())
        {
            view_updated = true;
        }

        if (!paused)
        {
            timer += timer_speed * frame_timer;
            if (timer > 1.0)
            {
                timer -= 1.0f;
            }
        }
        float fps_timer = static_cast<float>(std::chrono::duration<double, std::milli>(render_end - lastTimestamp).count());
        if (fps_timer > 1000.0f)
        {
            lastFPS = static_cast<uint32_t>(static_cast<float>(frame_counter) * (1000.0f / fps_timer));

            m_window.setTitle(GetWindowTitle().c_str());

            frame_counter = 0;
            lastTimestamp = render_end;
        }
    }

private:
    std::unique_ptr<Vulkan::IVulkanFactory> m_vulkan_factory;
    QVulkanWindow& m_window;
};

std::unique_ptr<IVulkanRenderer> IVulkanRenderer::Create(QVulkanWindow& window)
{
    return std::make_unique<VulkanRenderer>(window);
}
