#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan
{

struct VulkanShared;
struct Image;

class MappedData
{
public:
    MappedData(const Image& image, const VulkanShared& vulkan);
    ~MappedData();

    uint8_t*& GetData();

private:
    const VulkanShared& vulkan;

    VkDeviceMemory memory = nullptr;
    uint8_t*       data = nullptr;
};

}
