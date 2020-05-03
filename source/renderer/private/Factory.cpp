#include "IFactory.h"

#include <QVulkanWindow>
#include "IRenderer.h"

#include "Texture.h"
#include "Buffer.h"
#include "Shader.h"
#include "DescriptorSet.h"
#include "Pipeline.h"
#include "RenderPass.h"

#include <deque>

namespace Vulkan
{

class Factory
    : public IFactory
{
public:
    Factory(const QVulkanWindow& wnd)
        : window(wnd)
    {
    }

    ~Factory() override = default;

    std::unique_ptr<IRenderPass> CreateRenderPass() const override
    {
        return std::make_unique<RenderPass>(window);
    }

    ITexture& CreateTexture(const IDataProvider& data) override
    {
        textures.emplace_back(data, window);
        return textures.back();
    }

    IVertexBuffer& CreateVertexBuffer(const IDataProvider& data, const Attributes& attribs) override
    {
        vertex_buffers.emplace_back(data, attribs, window);
        return vertex_buffers.back();
    }

    IInstanceBuffer& CreateInstanceBuffer(const IDataProvider& data, const Attributes& attribs) override
    {
        instance_buffers.emplace_back(data, attribs, window);
        return instance_buffers.back();
    }

    IIndexBuffer& CreateIndexBuffer(const IDataProvider& data) override
    {
        index_buffers.emplace_back(data, window);
        return index_buffers.back();
    }

    IUniformBuffer& CreateUniformBuffer(const IDataProvider& data) override
    {
        uniform_buffers.emplace_back(data, window);
        return uniform_buffers.back();
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

    IPipeline& CreatePipeline(const IDescriptorSet& desc_set, const Shaders& shaders, const IVertexBuffer& vertex) override
    {
        pipelines.emplace_back(desc_set, shaders, vertex, window);
        return pipelines.back();
    }

    IPipeline& CreatePipeline(const IDescriptorSet& desc_set, const Shaders& shaders, const IVertexBuffer& vertex, const IInstanceBuffer& instance) override
    {
        pipelines.emplace_back(desc_set, shaders, vertex, instance, window);
        return pipelines.back();
    }

private:
    std::deque<Texture>        textures;
    std::deque<VertexBuffer>   vertex_buffers;
    std::deque<InstanceBuffer> instance_buffers;
    std::deque<IndexBuffer>    index_buffers;
    std::deque<UniformBuffer>  uniform_buffers;
    std::deque<Shader>         shaders;
    std::deque<DescriptorSet>  desc_sets;
    std::deque<Pipeline>       pipelines;

    const QVulkanWindow& window;
};

}

std::unique_ptr<Vulkan::IFactory> CreateFactory(const QVulkanWindow& window)
{
    return std::make_unique<Vulkan::Factory>(window);
}
