#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"
#include "IFactory.h"

#include <vector>

class QVulkanDeviceFunctions;
class QVulkanWindow;

namespace Vulkan
{
    struct Buffer
    {
        VkBuffer       buffer = nullptr;
        VkDeviceMemory memory = nullptr;
        uint32_t       size = 0u;
        bool           flush = false;
    };

    struct VertexLayout
    {
        VkPipelineVertexInputStateCreateInfo GetDesc() const;

        std::vector<VkVertexInputBindingDescription>   bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
    };

    class BufferBase
        : public IBuffer
    {
    public:
        BufferBase(const Buffer& buf, const QVulkanWindow&);
        ~BufferBase() override;

        void Update(const IDataProvider&) override;

    protected:
        QVulkanDeviceFunctions& functions;
        VkDevice device = nullptr;

        Buffer buffer{};
    };

    class VertexBuffer
        : public BufferBase
        , public IVertexBuffer
    {
    public:
        VertexBuffer(const IDataProvider&, const Attributes&, const QVulkanWindow&);
        ~VertexBuffer() override = default;

        void Update(const IDataProvider&) override;

        void Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const;
        const VertexLayout& GetLayout() const;

    private:
        VertexLayout layout{};
    };

    class IndexBuffer
        : public BufferBase
        , public IIndexBuffer
    {
    public:
        IndexBuffer(const IDataProvider&, const QVulkanWindow&);
        ~IndexBuffer() override = default;

        void Update(const IDataProvider&) override;

        void Bind(QVulkanDeviceFunctions& vulkan, VkCommandBuffer cmd_buf) const;

        uint32_t GetIndexCount() const;

    private:
        uint32_t index_count = 0;
    };

    class UniformBuffer
        : public BufferBase
        , public IUniformBuffer
    {
    public:
        UniformBuffer(const IDataProvider&, const QVulkanWindow&);
        ~UniformBuffer() override = default;

        void Update(const IDataProvider&) override;

        VkDescriptorBufferInfo GetInfo() const;
    };

}