#include "RenderInterface.h"
#include "VulkanFactory.h"
#include "Camera.h"

#include <QVulkanFunctions>
#include <QFile>
#include <QVector2D>

#include <array>
#include <iostream>
#include <sstream>

struct IvalidPath : public std::runtime_error
{
    IvalidPath(const std::string& msg)
        : std::runtime_error("Cannot find the path specified: " + msg)
    {
    }
};

struct InvalidVulkanCall : public std::logic_error
{
    InvalidVulkanCall()
        : std::logic_error("Invalid Vulkan Api usage")
    {
    }
};

void VkResultSuccess(VkResult res)
{
    if (res == VK_SUCCESS)
        return;
    throw InvalidVulkanCall();
}

struct IFrameCallback
{
    virtual void OnFrame(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) = 0;
    virtual ~IFrameCallback() = default;
};

class Texture
{
    const QVulkanWindow& m_window;

    static VkDeviceMemory Allocate(VkImage image, uint32_t index, const QVulkanWindow& window)
    {
        VkMemoryRequirements requerements = {};
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(window.device());
        device_functions.vkGetImageMemoryRequirements(window.device(), image, &requerements);

        if (!(requerements.memoryTypeBits & (1 << index)))
        {
            VkPhysicalDeviceMemoryProperties properties = {};
            window.vulkanInstance()->functions()->vkGetPhysicalDeviceMemoryProperties(window.physicalDevice(), &properties);
            for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
            {
                if (!(requerements.memoryTypeBits & (1 << i)))
                    continue;
                index = i;
            }
        }

        VkMemoryAllocateInfo allocate_info = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            nullptr,
            requerements.size,
            index
        };

        VkDeviceMemory res = nullptr;
        VkResultSuccess(device_functions.vkAllocateMemory(window.device(), &allocate_info, nullptr, &res));
        return res;
    }

    static void FillImageData(const QImage& img, VkImage image, VkDeviceMemory memory, const QVulkanWindow& window)
    {
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(window.device());

        VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
        VkSubresourceLayout layout;
        device_functions.vkGetImageSubresourceLayout(window.device(), image, &subres, &layout);

        uint8_t* mapped_data = nullptr;
        VkResultSuccess(device_functions.vkMapMemory(
            window.device(), memory, layout.offset, layout.size, 0, reinterpret_cast<void**>(&mapped_data))
        );

        for (int y = 0; y < img.height(); ++y)
        {
            memcpy(mapped_data, img.constScanLine(y), img.width() * 4);
            mapped_data += layout.rowPitch;
        }

        device_functions.vkUnmapMemory(window.device(), memory);
    }

    static VkImageCreateInfo GetImageCreateInfo(const QImage& img, VkImageTiling tiling, VkImageUsageFlags usage)
    {
        VkImageCreateInfo image_create_info = {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = format;
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = tiling;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        image_create_info.usage = usage;
        image_create_info.extent.width = img.width();
        image_create_info.extent.height = img.height();
        image_create_info.extent.depth = 1;
        return image_create_info;
    }

    static void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, const QVulkanWindow& window)
    {
        if (!commandBuffer)
            return;

        auto& device_functions = *window.vulkanInstance()->deviceFunctions(window.device());

        VkResultSuccess(device_functions.vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &commandBuffer;

        VkFenceCreateInfo fence_info = {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence fence = nullptr;

        VkResultSuccess(device_functions.vkCreateFence(window.device(), &fence_info, nullptr, &fence));
        VkResultSuccess(device_functions.vkQueueSubmit(queue, 1, &submit_info, fence));
        VkResultSuccess(device_functions.vkWaitForFences(window.device(), 1, &fence, VK_TRUE, 100000000000));

        device_functions.vkDestroyFence(window.device(), fence, nullptr);
        device_functions.vkFreeCommandBuffers(window.device(), pool, 1, &commandBuffer);
    }

    static VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, const QVulkanWindow& window)
    {
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(window.device());

        VkCommandBufferAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = window.graphicsCommandPool();
        allocate_info.level = level;
        allocate_info.commandBufferCount = 1;
        VkCommandBuffer cmd_buffer = nullptr;
        VkResultSuccess(device_functions.vkAllocateCommandBuffers(window.device(), &allocate_info, &cmd_buffer));

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResultSuccess(device_functions.vkBeginCommandBuffer(cmd_buffer, &begin_info));
        return cmd_buffer;
    }

    static VkSampler CreateSampler(const QVulkanWindow& window)
    {
        VkSamplerCreateInfo sampler = {};
        sampler.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.magFilter        = VK_FILTER_NEAREST;
        sampler.minFilter        = VK_FILTER_NEAREST;
        sampler.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.mipLodBias       = 0.0f;
        sampler.compareOp        = VK_COMPARE_OP_NEVER;
        sampler.minLod           = 0.0f;
        sampler.maxLod           = 0.0f;
        sampler.maxAnisotropy    = 1.0;
        sampler.anisotropyEnable = VK_FALSE;
        sampler.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VkSampler res = nullptr;
        VkResultSuccess(window.vulkanInstance()->deviceFunctions(window.device())->vkCreateSampler(
            window.device(), &sampler, nullptr, &res
        ));
        return res;
    }

    static VkImageView CreateImageView(VkImage image, const QVulkanWindow& window)
    {
        VkImageViewCreateInfo view = {};
        view.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view.format                          = format;
        view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.baseMipLevel   = 0;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount     = 1;
        view.subresourceRange.levelCount     = 1;
        view.image                           = image;

        view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        VkImageView res = nullptr;
        VkResultSuccess(window.vulkanInstance()->deviceFunctions(window.device())->vkCreateImageView(
            window.device(), &view, nullptr, &res
        ));
        return res;
    }

public:
    Texture(const QString& url, const QVulkanWindow& window)
        : m_window(window)
        , m_sampler(CreateSampler(window))
        , m_device(m_window.device())
        , m_vulkan_functions(*m_window.vulkanInstance()->functions())
        , m_device_functions(*m_window.vulkanInstance()->deviceFunctions(m_device))
    {
        QImage texure_image(url);
        if (texure_image.isNull())
            throw IvalidPath(url.toStdString());

        texure_image = texure_image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

        auto upload_create_info  = GetImageCreateInfo(texure_image, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        auto texture_create_info = GetImageCreateInfo(texure_image, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        VkImage upload_image = nullptr;
        VkResultSuccess(m_device_functions.vkCreateImage(m_device, &upload_create_info,  nullptr, &upload_image));
        VkDeviceMemory upload_buffer = Allocate(upload_image, window.hostVisibleMemoryIndex(), window);
        VkResultSuccess(m_device_functions.vkBindImageMemory(m_device, upload_image, upload_buffer, 0));
        FillImageData(texure_image, upload_image, upload_buffer, window);

        VkResultSuccess(m_device_functions.vkCreateImage(m_device, &texture_create_info, nullptr, &m_image));
        m_image_buffer = Allocate(m_image, window.deviceLocalMemoryIndex(), window);
        VkResultSuccess(m_device_functions.vkBindImageMemory(m_device, m_image, m_image_buffer, 0));

        VkCommandBuffer copy_cmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, window);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = barrier.subresourceRange.layerCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.image = upload_image;

        m_device_functions.vkCmdPipelineBarrier(copy_cmd,
            VK_PIPELINE_STAGE_HOST_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr,
            1, &barrier
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.image = m_image;
        m_device_functions.vkCmdPipelineBarrier(copy_cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &barrier
        );

        VkImageCopy copy_info = {};
        copy_info.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_info.srcSubresource.layerCount = 1;
        copy_info.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_info.dstSubresource.layerCount = 1;
        copy_info.extent.width  = texure_image.width();
        copy_info.extent.height = texure_image.height();
        copy_info.extent.depth = 1;
        m_device_functions.vkCmdCopyImage(copy_cmd,
            upload_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy_info);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.image = m_image;
        m_device_functions.vkCmdPipelineBarrier(copy_cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr,
            1, &barrier
        );

        FlushCommandBuffer(copy_cmd, window.graphicsQueue(), window.graphicsCommandPool(), window);

        m_view = CreateImageView(m_image, window);
    }

    VkDescriptorImageInfo GetInfo() const
    {
        VkDescriptorImageInfo info{};
        info.sampler = m_sampler;
        info.imageView = m_view;
        info.imageLayout = layout;
        return info;
    }

    ~Texture()
    {
        //auto device = m_window.device();
        //auto& device_functions = *m_window.vulkanInstance()->deviceFunctions(device);
        //
        //device_functions.vkDestroyImageView(device, m_view, nullptr);
        //device_functions.vkDestroyImage(device, m_image, nullptr);
        //device_functions.vkDestroySampler(device, m_sampler, nullptr);
        //device_functions.vkFreeMemory(device, m_image_buffer, nullptr);
    }

    static constexpr VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    static constexpr VkFormat      format = VK_FORMAT_R8G8B8A8_UNORM;
private:
    VkDevice m_device = nullptr;
    VkImage  m_image = nullptr;
    VkSampler m_sampler = nullptr;
    VkImageView m_view = nullptr;
    VkDeviceMemory m_image_buffer = nullptr;

    QVulkanFunctions&       m_vulkan_functions;
    QVulkanDeviceFunctions& m_device_functions;
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
    IndexedBuffer(IVulkanFactory& vulkan_factory)
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
    Buffer   m_index_buffer{};
    Buffer   m_vectex_buffer{};
};

class UniformBuffer
{
    struct {
        QMatrix4x4 projection;
        QMatrix4x4 model_view;
        QVector4D  view_pos;
    } m_ubo;

public:
    UniformBuffer(IVulkanFactory& vulkan_factory, const QVulkanWindow& window)
    {
        m_buffer = vulkan_factory.CreateBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(m_ubo),
            &m_ubo
        );
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);
        VkResultSuccess(device_functions.vkMapMemory(device, m_buffer.memory, 0, m_buffer.size, 0, reinterpret_cast<void**>(&mapped)));
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

    Buffer m_buffer{};
};

class DescriptorSet
    : public IFrameCallback
{
    VkDescriptorSetLayout m_descriptor_set_layout = nullptr;
    VkDescriptorSet       m_descriptor_set  = nullptr;
    VkDescriptorPool      m_descriptor_pool = nullptr;
    VkPipelineLayout      m_pipeline_layout = nullptr;
public:
    DescriptorSet(const Texture& texture, const UniformBuffer& uniform_buffer, const QVulkanWindow& window)
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

        VkResultSuccess(device_functions.vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptor_pool));

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

        VkResultSuccess(device_functions.vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, nullptr, &m_descriptor_set_layout));

        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &m_descriptor_set_layout;

        VkResultSuccess(device_functions.vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &m_pipeline_layout));

        VkDescriptorSetAllocateInfo descriptor_set_info{};
        descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptor_set_info.descriptorPool     = m_descriptor_pool;
        descriptor_set_info.pSetLayouts        = &m_descriptor_set_layout;
        descriptor_set_info.descriptorSetCount = 1;

        VkResultSuccess(device_functions.vkAllocateDescriptorSets(device, &descriptor_set_info, &m_descriptor_set));

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
        VkResultSuccess(device_functions.vkCreateShaderModule(device, &shader_info, nullptr, &res));
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

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = descriptor_set.GetPipelineLayout();
        pipelineCreateInfo.renderPass = window.defaultRenderPass();
        pipelineCreateInfo.flags = 0;
        pipelineCreateInfo.basePipelineIndex = -1;
        pipelineCreateInfo.basePipelineHandle = nullptr;

        auto vertex_info = vertex.GetVertexLayoutInfo();
        pipelineCreateInfo.pVertexInputState    = &vertex_info;
        pipelineCreateInfo.pInputAssemblyState  = &input_assembly_state_info;
        pipelineCreateInfo.pRasterizationState  = &rasterization_state_info;
        pipelineCreateInfo.pColorBlendState     = &color_blend_state_info;
        pipelineCreateInfo.pMultisampleState    = &multisample_state_info;
        pipelineCreateInfo.pViewportState       = &viewport_state_info;
        pipelineCreateInfo.pDepthStencilState   = &depth_stencil_state_info;
        pipelineCreateInfo.pDynamicState        = &dynamic_state_info;
        pipelineCreateInfo.stageCount           = static_cast<uint32_t>(shader_stages.size());
        pipelineCreateInfo.pStages              = shader_stages.data();

        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

        VkResultSuccess(device_functions.vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_pipeline_cache));
        VkResultSuccess(device_functions.vkCreateGraphicsPipelines(device, m_pipeline_cache, 1, &pipelineCreateInfo, nullptr, &m_pipeline));
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
    std::unique_ptr<Texture>       m_texture;
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
        m_vulkan_factory = IVulkanFactory::Create(m_window);
        m_texture = std::make_unique<Texture>(":/dirt.png", m_window);
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
    std::unique_ptr<IVulkanFactory> m_vulkan_factory;
    QVulkanWindow& m_window;
};

std::unique_ptr<IVulkanRenderer> IVulkanRenderer::Create(QVulkanWindow& window)
{
    return std::make_unique<VulkanRenderer>(window);
}
