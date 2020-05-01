#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan
{

struct IVulkanFactory;
struct IMappedData;

struct ITextureSource
{
    virtual void FillMappedData(const IMappedData& data) const = 0;
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
