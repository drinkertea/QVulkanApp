#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"
#include "IFactory.h"

#include <vector>
#include <deque>

namespace Vulkan
{

struct VulkanShared;

struct BufferDesc
{
    VkBuffer           buffer = nullptr;
    VkDeviceMemory     memory = nullptr;
    uint32_t           size = 0u;
    bool               flush = false;
};

struct VertexBinding
    : public IVertexBinding
{
    VertexBinding(uint32_t binding, uint32_t location_offset, VkVertexInputRate input_rate);
    ~VertexBinding() override = default;

    void AddAttribute(AttributeFormat format) override;

    using Attributes = std::vector<VkVertexInputAttributeDescription>;
    const Attributes& GetAttributes() const { return attributes; }

    VkVertexInputBindingDescription GetDesc() const;

private:
    uint32_t GetSize() const;

    uint32_t          binding = 0u;
    uint32_t          location_offset = 0u;
    VkVertexInputRate input_rate;
    Attributes        attributes;
};

struct VertexLayoutData
{
    VkPipelineVertexInputStateCreateInfo GetDesc() const;
    std::vector<VkVertexInputBindingDescription>   bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct VertexLayout
    : public IVertexLayout
{
    ~VertexLayout() override = default;
    IVertexBinding& AddVertexBinding() override;
    IVertexBinding& AddInstanceBinding() override;

    VertexLayoutData GetData() const;

private:
    std::deque<VertexBinding> bindings;
};

class Buffer
    : public IBuffer
{
public:
    Buffer(BufferUsage usage, const IDataProvider& data, VulkanShared& vulkan);
    ~Buffer() override;

    void Update(const IDataProvider&) override;

    void Bind(VkCommandBuffer cmd_buf) const;

    uint32_t GetWidth() const { return width; }
    BufferUsage GetUsage() const { return usage; }

protected:
    VulkanShared& vulkan;

    uint32_t    width = 0u;
    BufferUsage usage{};
    BufferDesc  buffer{};
};

}