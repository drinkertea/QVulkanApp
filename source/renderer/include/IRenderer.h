#pragma once

#include <cstdint>

namespace Vulkan
{

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

struct IIndexBuffer : public IBuffer
{
    virtual ~IIndexBuffer() = default;
};

struct IVertexBuffer : public IBuffer
{
    virtual ~IVertexBuffer() = default;
};

struct IInstanceBuffer : public IBuffer
{
    virtual ~IInstanceBuffer() = default;
};

struct IUniformBuffer
    : public IInputResource
    , public IBuffer
{
    virtual ~IUniformBuffer() = default;
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

struct IRenderPass
{
    virtual void Bind(const IDescriptorSet&) const = 0;
    virtual void Bind(const IPipeline&) const = 0;
    virtual void Draw(const IIndexBuffer&, const IVertexBuffer&) const = 0;
    virtual void Bind(const IIndexBuffer&, const IVertexBuffer&) const = 0;
    virtual void Draw(const IInstanceBuffer&) const = 0;
    virtual ~IRenderPass() = default;
};

}
