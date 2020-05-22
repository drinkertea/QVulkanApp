#pragma once

#include "IRenderer.h"

#include <functional>
#include <vector>
#include <memory>

namespace Vulkan
{

struct ICamera;

enum class BufferUsage
{
    Index = 0,
    Vertex,
    Instance,
};

using Attributes = std::vector<AttributeFormat>;
using InputResources = std::vector<std::reference_wrapper<const IInputResource>>;
using Shaders = std::vector<std::reference_wrapper<const IShader>>;

struct IFactory
{
    virtual uint32_t GetFrameBufferCount() const = 0;

    virtual std::unique_ptr<IRenderPass> CreateRenderPass(ICamera& camera) const = 0;

    virtual std::unique_ptr<IBuffer> CreateBuffer(BufferUsage usage, const IDataProvider&) = 0;

    virtual ITexture& CreateTexture(const IDataProvider&) = 0;
    virtual IBuffer& AddBuffer(BufferUsage usage, const IDataProvider&) = 0;
    virtual IVertexLayout& AddVertexLayout() = 0;

    virtual IShader& CreateShader(const IDataProvider&, ShaderType) = 0;
    virtual IDescriptorSet& CreateDescriptorSet(const InputResources&) = 0;
    virtual IPipeline& CreatePipeline(const IDescriptorSet&, const Shaders&, const IVertexLayout&) = 0;
    virtual ~IFactory() = default;
};

}

class QVulkanWindow;
std::unique_ptr<Vulkan::IFactory> CreateFactory(const QVulkanWindow&);
