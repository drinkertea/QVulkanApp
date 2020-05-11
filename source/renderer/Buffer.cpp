#include "Buffer.h"
#include "Utils.h"

#include "ScopeCommandBuffer.h"

#include <QVulkanFunctions>
#include <QVulkanWindow>

namespace Vulkan
{
    uint32_t GetMemoryIndex(uint32_t type_bits, VkMemoryPropertyFlags memory_property, const VkPhysicalDeviceMemoryProperties& memory_properties)
    {
        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        {
            if ((type_bits & 1) == 1 && (memory_properties.memoryTypes[i].propertyFlags & memory_property) == memory_property)
                return i;
            type_bits >>= 1;
        }
        throw std::runtime_error("Could not find a matching memory type");
    }

    VkDeviceMemory Allocate(uint32_t index, VkDeviceSize size, const QVulkanWindow& window);

    Buffer CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_property, VkDeviceSize size, const QVulkanWindow& window)
    {
        auto device = window.device();
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(device);
        Buffer buffer = {};
        buffer.size = size;
        buffer.flush = (memory_property & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0;

        VkBufferCreateInfo buffer_createInfo{};
        buffer_createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_createInfo.usage = usage;
        buffer_createInfo.size = size;

        VkResultSuccess(device_functions.vkCreateBuffer(device, &buffer_createInfo, nullptr, &buffer.buffer));

        VkMemoryRequirements memory_requiments{};
        device_functions.vkGetBufferMemoryRequirements(device, buffer.buffer, &memory_requiments);

        VkPhysicalDeviceMemoryProperties mem_props = {};
        window.vulkanInstance()->functions()->vkGetPhysicalDeviceMemoryProperties(
            window.physicalDevice(), &mem_props
        );

        buffer.memory = Allocate(
            GetMemoryIndex(memory_requiments.memoryTypeBits, memory_property, mem_props),
            memory_requiments.size,
            window
        );

        VkResultSuccess(device_functions.vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0));

        return buffer;
    }

    VkFormat AttributeToFormat(AttributeFormat attribute)
    {
        switch (attribute)
        {
            case AttributeFormat::vec1f: return VK_FORMAT_R32_SFLOAT;
            case AttributeFormat::vec2f: return VK_FORMAT_R32G32_SFLOAT;
            case AttributeFormat::vec3f: return VK_FORMAT_R32G32B32_SFLOAT;
            case AttributeFormat::vec4f: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case AttributeFormat::vec1i: return VK_FORMAT_R32_SINT;
            default: throw std::logic_error("Wrong enum value");
        }
    }

    uint32_t AttributeSize(AttributeFormat attribute)
    {
        switch (attribute)
        {
        case AttributeFormat::vec1f: return sizeof(float) * 1;
        case AttributeFormat::vec2f: return sizeof(float) * 2;
        case AttributeFormat::vec3f: return sizeof(float) * 3;
        case AttributeFormat::vec4f: return sizeof(float) * 4;
        case AttributeFormat::vec1i: return sizeof(int32_t) * 1;
        default: throw std::logic_error("Wrong enum value");
        }
    }

    static constexpr uint32_t vertex_binding_index = 0;
    static constexpr uint32_t instance_binding_index = 1;

    BufferBase::BufferBase(const Buffer& buf, const QVulkanWindow& window)
        : device(window.device())
        , functions(*window.vulkanInstance()->deviceFunctions(window.device()))
        , buffer(buf)
    {
    }
    BufferBase::~BufferBase()
    {
        functions.vkDestroyBuffer(device, buffer.buffer, nullptr);
        functions.vkFreeMemory(device, buffer.memory, nullptr);
    }

    void BufferBase::Update(const IDataProvider& data)
    {
        if (!data.GetData())
            return;

        void* mapped = nullptr;
        VkResultSuccess(functions.vkMapMemory(device, buffer.memory, 0, buffer.size, 0, &mapped));
        memcpy_s(mapped, buffer.size, data.GetData(), data.GetSize());
        if (buffer.flush)
        {
            VkMappedMemoryRange mapped_range = {};
            mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mapped_range.memory = buffer.memory;
            mapped_range.offset = 0;
            mapped_range.size = buffer.size;
            VkResultSuccess(functions.vkFlushMappedMemoryRanges(device, 1, &mapped_range));
        }
        functions.vkUnmapMemory(device, buffer.memory);
    }

    VertexLayout CreateLayout(uint32_t binding, const Attributes& attributes)
    {
        VertexLayout layout{};
        VkVertexInputAttributeDescription attribute_desc{};
        attribute_desc.binding = binding;

        uint32_t size = 0;
        for (uint32_t i = 0; i < attributes.size(); ++i)
        {
            attribute_desc.location = i;
            attribute_desc.format = AttributeToFormat(attributes[i]);
            attribute_desc.offset = size;
            size += AttributeSize(attributes[i]);
            layout.attributes.emplace_back(attribute_desc);
        }

        VkVertexInputBindingDescription binding_desc{};
        binding_desc.binding = binding;
        binding_desc.stride = size;
        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        layout.bindings.emplace_back(binding_desc);
        return layout;
    }

    VertexBuffer::VertexBuffer(const IDataProvider& data, const Attributes& attributes, const QVulkanWindow& window)
        : BufferBase(CreateBuffer(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            data.GetSize(),
            window
        ), window)
        , window(window)
        , layout(CreateLayout(vertex_binding_index, attributes))
    {
        VertexBuffer::Update(data);
    }

    void VertexBuffer::Update(const IDataProvider& data)
    {
        auto upload = CreateBuffer(
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            data.GetSize(),
            window
        );

        BufferBase bb(upload, window);
        bb.Update(data);

        ScopeCommandBuffer scb(window);
        VkBufferCopy info = {};
        info.size = data.GetSize();
        scb.CopyBuffer(upload.buffer, buffer.buffer, info);
    }

    void VertexBuffer::Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const
    {
        VkDeviceSize offsets[1] = { 0 };
        vulkan.vkCmdBindVertexBuffers(cmd_buf, vertex_binding_index, 1, &buffer.buffer, offsets);
    }

    const VertexLayout& VertexBuffer::GetLayout() const
    {
        return layout;
    }

    VkPipelineVertexInputStateCreateInfo VertexLayout::GetDesc() const
    {
        VkPipelineVertexInputStateCreateInfo desc{};
        desc.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        desc.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        desc.pVertexBindingDescriptions = bindings.data();
        desc.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        desc.pVertexAttributeDescriptions = attributes.data();
        return desc;
    }

    IndexBuffer::IndexBuffer(const IDataProvider& data, const QVulkanWindow& window)
        : BufferBase(CreateBuffer(
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            data.GetSize(),
            window
        ), window)
        , index_count(data.GetWidth())
    {
        BufferBase::Update(data);
    }

    void IndexBuffer::Update(const IDataProvider& data)
    {
        BufferBase::Update(data);
    }

    void IndexBuffer::Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const
    {
        vulkan.vkCmdBindIndexBuffer(cmd_buf, buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    uint32_t IndexBuffer::GetIndexCount() const
    {
        return index_count;
    }

    UniformBuffer::UniformBuffer(const IDataProvider& data, const QVulkanWindow& window)
        : BufferBase(CreateBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            data.GetSize(),
            window
        ), window)
    {
        BufferBase::Update(data);
    }

    void UniformBuffer::Update(const IDataProvider& data)
    {
        BufferBase::Update(data);
    }

    VkDescriptorBufferInfo UniformBuffer::GetInfo() const
    {
        VkDescriptorBufferInfo info{};
        info.buffer = buffer.buffer;
        info.offset = 0;
        info.range  = VK_WHOLE_SIZE;
        return info;
    }

    InstanceBuffer::InstanceBuffer(const IDataProvider& data, const Attributes& attributes, const QVulkanWindow& window)
        : VertexBuffer(data, attributes, window)
        , instance_count(data.GetWidth())
    {
        layout = CreateLayout(instance_binding_index, attributes);
        layout.bindings.front().inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    }

    void InstanceBuffer::Update(const IDataProvider& data)
    {
        VertexBuffer::Update(data);
    }

    void InstanceBuffer::Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const
    {
        VkDeviceSize offsets[1] = { 0 };
        vulkan.vkCmdBindVertexBuffers(cmd_buf, instance_binding_index, 1, &buffer.buffer, offsets);
    }
    uint32_t InstanceBuffer::GetInstanceCount() const
    {
        return instance_count;
    }
}