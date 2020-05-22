#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"

namespace Vulkan
{

struct VulkanShared;

struct Image
{
    VkImage        image = nullptr;
    VkDeviceMemory memory = nullptr;
};

class Texture
    : public ITexture
{
public:
    Texture(const IDataProvider& data, VulkanShared& vulkan, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    ~Texture() override;

    VkDescriptorImageInfo GetInfo() const;

private:
    VulkanShared& vulkan;

    Image image{};
    VkSampler      sampler = nullptr;
    VkImageView    view    = nullptr;
};

}