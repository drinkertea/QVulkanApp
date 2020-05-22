#include "Buffer.h"

#include "ScopeCommandBuffer.h"
#include "Common.h"
#include "Utils.h"

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

VkDeviceMemory Allocate(uint32_t index, VkDeviceSize size, VkDevice device)
{
    VkMemoryAllocateInfo allocate_info = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        size,
        index
    };

    VkDeviceMemory res = nullptr;
    VkResultSuccess(vkAllocateMemory(device, &allocate_info, nullptr, &res));
    return res;
}

BufferDesc CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_property, VkDeviceSize size, VulkanShared& vulkan)
{
    BufferDesc buffer = {};
    buffer.size = static_cast<uint32_t>(size);
    buffer.flush = (memory_property & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0;

    VkBufferCreateInfo buffer_createInfo{};
    buffer_createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_createInfo.usage = usage;
    buffer_createInfo.size = size;

    VkResultSuccess(vkCreateBuffer(vulkan.device, &buffer_createInfo, nullptr, &buffer.buffer));

    VkMemoryRequirements memory_requiments{};
    vkGetBufferMemoryRequirements(vulkan.device, buffer.buffer, &memory_requiments);

    VkPhysicalDeviceMemoryProperties mem_props = {};
    vkGetPhysicalDeviceMemoryProperties(
        vulkan.physical_device, &mem_props
    );

    buffer.memory = Allocate(
        GetMemoryIndex(memory_requiments.memoryTypeBits, memory_property, mem_props),
        memory_requiments.size,
        vulkan.device
    );

    VkResultSuccess(vkBindBufferMemory(vulkan.device, buffer.buffer, buffer.memory, 0));

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

uint32_t AttributeSize(VkFormat attribute)
{
    switch (attribute)
    {
    case VK_FORMAT_R32_SFLOAT:          return sizeof(float) * 1;
    case VK_FORMAT_R32G32_SFLOAT:       return sizeof(float) * 2;
    case VK_FORMAT_R32G32B32_SFLOAT:    return sizeof(float) * 3;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return sizeof(float) * 4;
    case VK_FORMAT_R32_SINT:            return sizeof(int32_t) * 1;
    default: throw std::logic_error("Wrong enum value");
    }
}

VkBufferUsageFlags VkBufferUsage(BufferUsage usage)
{
    switch (usage)
    {
    case BufferUsage::Index:    return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case BufferUsage::Vertex:
    case BufferUsage::Instance: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    default: throw std::logic_error("Wrong enum value");
    }
}

static constexpr uint32_t vertex_binding_index = 0;
static constexpr uint32_t instance_binding_index = 1;

Buffer::Buffer(BufferUsage usage, const IDataProvider& data, VulkanShared& vulkan)
    : vulkan(vulkan)
    , usage(usage)
    , width(data.GetWidth())
    , buffer(CreateBuffer(
        VkBufferUsage(usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        data.GetSize(),
        vulkan
    ))
{
    Update(data);
}

Buffer::~Buffer()
{
    vkDestroyBuffer(vulkan.device, buffer.buffer, nullptr);
    vkFreeMemory(vulkan.device, buffer.memory, nullptr);
}

void Buffer::Update(const IDataProvider& data)
{
    if (!data.GetData())
        return;

    auto upload = CreateBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        data.GetSize(),
        vulkan
    );

    void* mapped = nullptr;
    VkResultSuccess(vkMapMemory(vulkan.device, upload.memory, 0, upload.size, 0, &mapped));
    memcpy_s(mapped, upload.size, data.GetData(), data.GetSize());
    if (upload.flush)
    {
        VkMappedMemoryRange mapped_range = {};
        mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.memory = upload.memory;
        mapped_range.offset = 0;
        mapped_range.size = upload.size;
        VkResultSuccess(vkFlushMappedMemoryRanges(vulkan.device, 1, &mapped_range));
    }
    vkUnmapMemory(vulkan.device, upload.memory);

    {
        ScopeCommandBuffer scb(vulkan);
        VkBufferCopy info = {};
        info.size = data.GetSize();
        scb.CopyBuffer(upload.buffer, buffer.buffer, info);
    }

    vkDestroyBuffer(vulkan.device, upload.buffer, nullptr);
    vkFreeMemory(vulkan.device, upload.memory, nullptr);
}

void Buffer::Bind(VkCommandBuffer cmd_buf) const
{
    VkDeviceSize offsets[1] = { 0 };

    switch (usage)
    {
    case BufferUsage::Index:
        return vkCmdBindIndexBuffer(cmd_buf, buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    case BufferUsage::Vertex:
        return vkCmdBindVertexBuffers(cmd_buf, vertex_binding_index, 1, &buffer.buffer, offsets);
    case BufferUsage::Instance:
        return vkCmdBindVertexBuffers(cmd_buf, instance_binding_index, 1, &buffer.buffer, offsets);
    default: throw std::logic_error("Wrong enum value");
    }
}

VertexBinding::VertexBinding(uint32_t binding, uint32_t location_offset, VkVertexInputRate input_rate)
    : binding(binding)
    , input_rate(input_rate)
    , location_offset(location_offset)
{
}

void VertexBinding::AddAttribute(AttributeFormat format)
{
    attributes.push_back({
        .location = location_offset + static_cast<uint32_t>(attributes.size()),
        .binding = binding,
        .format = AttributeToFormat(format),
        .offset = GetSize()
    });
}

VkVertexInputBindingDescription VertexBinding::GetDesc() const
{
    return {
        .binding = binding,
        .stride = GetSize(),
        .inputRate = input_rate
    };
}

uint32_t VertexBinding::GetSize() const
{
    if (attributes.empty())
        return 0u;
    return attributes.back().offset + AttributeSize(attributes.back().format);
}

VkPipelineVertexInputStateCreateInfo VertexLayoutData::GetDesc() const
{
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size()),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data(),
    };
}

IVertexBinding& VertexLayout::AddVertexBinding()
{
    bindings.emplace_back(
        static_cast<uint32_t>(bindings.size()),
        bindings.empty() ? 0u : static_cast<uint32_t>(bindings.back().GetAttributes().size()),
        VK_VERTEX_INPUT_RATE_VERTEX
    );
    return bindings.back();
}

IVertexBinding& VertexLayout::AddInstanceBinding()
{
    bindings.emplace_back(
        static_cast<uint32_t>(bindings.size()),
        bindings.empty() ? 0u : static_cast<uint32_t>(bindings.back().GetAttributes().size()),
        VK_VERTEX_INPUT_RATE_INSTANCE
    );
    return bindings.back();
}

VertexLayoutData VertexLayout::GetData() const
{
    VertexLayoutData layout_data;
    layout_data.bindings.reserve(bindings.size());
    for (const auto& binding : bindings)
    {
        layout_data.bindings.push_back(binding.GetDesc());
        layout_data.attributes.insert(layout_data.attributes.end(), binding.GetAttributes().begin(), binding.GetAttributes().end());
    }
    return layout_data;
}

}
