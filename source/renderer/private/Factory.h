#pragma once

#include <QVulkanWindow>

#include "IRenderer.h"

namespace Vulkan
{

class Factory
    : public IFactory
{
    ITexture&       CreateTexture(const IDataProvider&)        override;
    IVertexBuffer&  CreateVertexBuffer(const IDataProvider&)   override;
    IIndexBuffer&   CreateIndexBuffer(const IDataProvider&)    override;
    IUniformBuffer& CreateUniformBuffer(const IDataProvider&)  override;
    IDescriptorSet& CreateDescriptorSet(const InputResources&) override;

    IPipeline&      CreatePipeline(const IDescriptorSet&, const Shaders&) override;

    ~Factory() override;
};

}
