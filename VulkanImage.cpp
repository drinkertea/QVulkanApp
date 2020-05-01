#include "VulkanImage.h"
#include "VulkanFactory.h"

namespace Vulkan
{

Texture::Texture(const IVulkanFactory& factory, const ITextureSource& source, VkFormat format)
    : image(factory.Create2dImage(format, source.GetWidth(), source.GetHeight()))
    , sampler(factory.CreateSampler())
    , view(factory.CreateImageView(image.image, format))
{
    auto upload_image = factory.CreateUploadImage(format, source.GetWidth(), source.GetHeight());

    auto mapped_data = factory.MapImage(upload_image);
    source.FillMappedData(*mapped_data);

    VkExtent3D extent = { source.GetWidth(), source.GetHeight(), 1 };
    factory.CopyImage(upload_image, image, extent);
}

VkDescriptorImageInfo Texture::GetInfo() const
{
    VkDescriptorImageInfo info{};
    info.sampler     = sampler;
    info.imageView   = view;
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return info;
}

}