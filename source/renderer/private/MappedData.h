#pragma once

#include <vulkan/vulkan.h>

class QVulkanWindow;

namespace Vulkan
{

struct Image;

class MappedData
{
public:
    MappedData(const Image& image, const QVulkanWindow& window);
    ~MappedData();

    uint8_t*& GetData();

private:
    const QVulkanWindow& m_window;

    VkDeviceMemory memory = nullptr;
    uint8_t*       data = nullptr;
};

}
