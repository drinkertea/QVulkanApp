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
    virtual ~IInputResource() = default;
};

struct ITexture : public IInputResource
{
    virtual ~ITexture() = default;
};

struct IBuffer
{
    virtual void Update(const IDataProvider&) = 0;
    virtual ~IBuffer() = default;
};

struct IIndexBuffer
{
    virtual ~IIndexBuffer() = default;
};

struct IVertexBuffer
{
    virtual ~IVertexBuffer() = default;
};

struct IUniformBuffer
    : public IInputResource
{
    virtual ~IUniformBuffer() = default;
};

struct IDescriptorSet
{
    virtual ~IDescriptorSet() = default;
};

struct IShader
{
    virtual ~IShader() = default;
};

struct IPipeline
{
    virtual ~IPipeline() = default;
};

struct IRenderPass
{
    virtual void Bind(const IDescriptorSet&) const = 0;
    virtual void Bind(const IPipeline&) const = 0;
    virtual void Draw(const IIndexBuffer&, const IVertexBuffer&) const = 0;
    virtual ~IRenderPass() = default;
};

}
