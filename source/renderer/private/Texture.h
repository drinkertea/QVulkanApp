#pragma once

#include <QVulkanWindow>

#include "IRenderer.h"

namespace Vulkan
{
    struct Image
    {
        VkImage        image = nullptr;
        VkDeviceMemory memory = nullptr;
    };

    class Texture
        : public ITexture
    {
    public:
        Texture(const IDataProvider& data, const QVulkanWindow& window, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
        ~Texture() override;

        VkDescriptorImageInfo GetInfo() const;

    private:
        QVulkanDeviceFunctions& functions;
        VkDevice device = nullptr;

        Image image{};
        VkSampler      sampler = nullptr;
        VkImageView    view    = nullptr;
    };

}