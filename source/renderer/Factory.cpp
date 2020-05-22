#include "IFactory.h"

#include "Common.h"
#include <QVulkanWindow>
#include "IRenderer.h"

#include "Texture.h"
#include "Buffer.h"
#include "Shader.h"
#include "DescriptorSet.h"
#include "Pipeline.h"
#include "RenderPass.h"

#include <deque>
#include <set>

namespace Vulkan
{

class Factory
    : public IFactory
{
public:
    Factory(const QVulkanWindow& window)
        : window(window)
        , vulkan({
            .device          = window.device(),
            .physical_device = window.physicalDevice(),
            .command_pool    = window.graphicsCommandPool(),
            .graphics_queue  = window.graphicsQueue(),
            .render_pass     = window.defaultRenderPass(),
            .host_memory_index   = window.hostVisibleMemoryIndex(),
            .device_memory_index = window.deviceLocalMemoryIndex(),
        })
    {
    }

    ~Factory() override = default;

    uint32_t GetFrameBufferCount() const override
    {
        return static_cast<uint32_t>(window.concurrentFrameCount());
    }

    std::unique_ptr<IRenderPass> CreateRenderPass(ICamera& camera) const override
    {
        return std::make_unique<RenderPass>(camera, window);
    }

    ITexture& CreateTexture(const IDataProvider& data) override
    {
        textures.emplace_back(data, vulkan);
        return textures.back();
    }

    IBuffer& AddBuffer(BufferUsage usage, const IDataProvider& data) override
    {
        buffers.emplace_back(usage, data, vulkan);
        return buffers.back();
    }

    IVertexLayout& AddVertexLayout() override
    {
        vertex_layouts.emplace_back();
        return vertex_layouts.back();
    }

    std::unique_ptr<IBuffer> CreateBuffer(BufferUsage usage, const IDataProvider& data) override
    {
        return std::make_unique<Buffer>(usage, data, vulkan);
    }

    IShader& CreateShader(const IDataProvider& data, ShaderType type) override
    {
        shaders.emplace_back(data, type, window);
        return shaders.back();
    }

    IDescriptorSet& CreateDescriptorSet(const InputResources& bindings) override
    {
        desc_sets.emplace_back(bindings, window);
        return desc_sets.back();
    }

    IPipeline& CreatePipeline(const IDescriptorSet& desc_set, const Shaders& shaders, const IVertexLayout& vertex) override
    {
        const auto& desc_set_impl = dynamic_cast<const DescriptorSet&>(desc_set);
        const auto& vertex_layout = dynamic_cast<const VertexLayout&>(vertex);
        pipelines.emplace_back(desc_set_impl, shaders, vertex_layout, vulkan);
        return pipelines.back();
    }

private:
    std::deque<Texture>        textures;
    std::deque<Buffer>         buffers;
    std::deque<VertexLayout>   vertex_layouts;
    std::deque<Shader>         shaders;
    std::deque<DescriptorSet>  desc_sets;
    std::deque<Pipeline>       pipelines;

    VulkanShared vulkan;

    const QVulkanWindow& window;
};

}

std::unique_ptr<Vulkan::IFactory> CreateFactory(const QVulkanWindow& window)
{
    return std::make_unique<Vulkan::Factory>(window);
}
