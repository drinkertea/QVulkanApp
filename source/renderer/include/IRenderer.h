#pragma once

#include <cstdint>

namespace Vulkan
{

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

struct IDataProvider
{
    virtual const uint8_t* GetData() const = 0;
    virtual uint32_t GetSize()   const = 0;
    virtual uint32_t GetWidth()  const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual uint32_t GetDepth()  const = 0;
    virtual ~IDataProvider() = default;
};

struct IInputResource
{
    virtual ~IInputResource() = 0 {}
};

struct ITexture : public IInputResource
{
    virtual ~ITexture() = 0 {}
};

struct IBuffer
{
    virtual void Update(const IDataProvider&) = 0;
    virtual ~IBuffer() = default;
};

struct IPushConstantLayout
    : public IInputResource
{
    virtual ~IPushConstantLayout() = 0 {}
};

struct IDescriptorSet
{
    virtual ~IDescriptorSet() = 0 {}
};

struct IShader
{
    virtual ~IShader() = 0 {}
};

struct IPipeline
{
    virtual ~IPipeline() = 0 {}
};

struct IVertexBinding
{
    virtual void AddAttribute(AttributeFormat format) = 0;
    virtual ~IVertexBinding() = default;
};

struct IVertexLayout
{
    virtual IVertexBinding& AddVertexBinding() = 0;
    virtual IVertexBinding& AddInstanceBinding() = 0;
    virtual ~IVertexLayout() = default;
};

struct ICommandBuffer
{
    virtual void Bind(const IDescriptorSet&) const = 0;
    virtual void Bind(const IPipeline&) const = 0;
    virtual void Bind(const IBuffer&) const = 0;
    virtual void Draw(const IBuffer&, uint32_t count = 0, uint32_t offset = 0) const = 0;
    virtual ~ICommandBuffer() = default;
};

struct IRenderPass
{
    virtual void AddCommandBuffer(ICommandBuffer&) = 0;
    virtual ~IRenderPass() = default;
};

}
