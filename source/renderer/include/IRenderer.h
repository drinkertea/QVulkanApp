#pragma once

#include <cstdint>

#include <functional>
#include <vector>

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
    using InputResources = std::vector<std::reference_wrapper<const IInputResource>>;

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

    enum class AttributeFormat
    {
        vec1f = 0,
        vec2f,
        vec3f,
        vec4f,
    };
    using Attributes = std::vector<AttributeFormat>;

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

    enum class ShaderType
    {
        vertex = 0,
        fragment,
    };

    struct IShader
    {
        virtual ~IShader() = default;
    };
    using Shaders = std::vector<std::reference_wrapper<const IShader>>;

    struct IPipeline
    {
        virtual ~IPipeline() = default;
    };

    struct ICommandBuffer
    {
        virtual void Bind(const IDescriptorSet&) = 0;
        virtual void Bind(const IPipeline&) = 0;
        virtual void Draw(const IIndexBuffer&, const IVertexBuffer&) = 0;
        virtual ~ICommandBuffer() = default;
    };

    struct IFactory
    {
        virtual ITexture&       CreateTexture      (const IDataProvider&) = 0;
        virtual IVertexBuffer&  CreateVertexBuffer (const IDataProvider&, const Attributes&) = 0;
        virtual IIndexBuffer&   CreateIndexBuffer  (const IDataProvider&) = 0;
        virtual IUniformBuffer& CreateUniformBuffer(const IDataProvider&) = 0;
        virtual IShader&        CreateShader       (const IDataProvider&, ShaderType) = 0;
        virtual IDescriptorSet& CreateDescriptorSet(const InputResources&) = 0;
        virtual IPipeline&      CreatePipeline     (const IDescriptorSet&, const Shaders&) = 0;
        virtual ~IFactory() = default;
    };
}
