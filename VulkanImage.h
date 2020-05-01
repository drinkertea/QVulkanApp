#pragma once

#include <vulkan/vulkan.h>

struct IVulkanFactory;
struct IMappedData;

namespace Vulkan
{

struct ITextureSource
{
    virtual void FillMappedData(const IMappedData&) const = 0;
    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual ~ITextureSource() = default;
};

struct Image
{
    VkImage        image = nullptr;
    VkDeviceMemory memory = nullptr;
};

struct Texture
{
public:
    Texture(
        const IVulkanFactory& factory,
        const ITextureSource& source,
        VkFormat format
    );

    VkDescriptorImageInfo GetInfo() const;

private:
    Image       image{};
    VkSampler   sampler = nullptr;
    VkImageView view    = nullptr;
};

}
