#pragma once

#include "IRenderer.h"

#include <functional>
#include <vector>
#include <memory>

namespace Vulkan
{

struct ICamera;

enum class ShaderType
{
    vertex = 0,
    fragment,
};

enum class AttributeFormat
{
    vec1f = 0,
    vec2f,
    vec3f,
    vec4f,
    vec1i,
};

using Attributes = std::vector<AttributeFormat>;
using InputResources = std::vector<std::reference_wrapper<const IInputResource>>;
using Shaders = std::vector<std::reference_wrapper<const IShader>>;

struct IFactory
{
    virtual std::unique_ptr<IRenderPass> CreateRenderPass(ICamera& camera) const = 0;

    virtual std::unique_ptr<IInstanceBuffer> CreateInstanceBuffer(const IDataProvider&, const Attributes&) = 0;

    virtual ITexture& CreateTexture(const IDataProvider&) = 0;
    virtual IVertexBuffer& CreateVertexBuffer(const IDataProvider&, const Attributes&) = 0;
    virtual IIndexBuffer& CreateIndexBuffer(const IDataProvider&) = 0;
    virtual IUniformBuffer& CreateUniformBuffer(const IDataProvider&) = 0;

    virtual IShader& CreateShader(const IDataProvider&, ShaderType) = 0;
    virtual IDescriptorSet& CreateDescriptorSet(const InputResources&) = 0;
    virtual IPipeline& CreatePipeline(const IDescriptorSet&, const Shaders&, const IVertexBuffer&) = 0;
    virtual IPipeline& CreatePipeline(const IDescriptorSet&, const Shaders&, const IVertexBuffer&, const IInstanceBuffer&) = 0;
    virtual ~IFactory() = default;
};

}

class QVulkanWindow;
std::unique_ptr<Vulkan::IFactory> CreateFactory(const QVulkanWindow&);
