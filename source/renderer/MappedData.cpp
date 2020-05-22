#include "MappedData.h"
#include "Common.h"
#include "Texture.h"
#include "Utils.h"

namespace Vulkan
{
    MappedData::MappedData(const Image& image, const VulkanShared& vulkan)
        : vulkan(vulkan)
        , memory(image.memory)
    {
        VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
        VkSubresourceLayout layout;
        vkGetImageSubresourceLayout(vulkan.device, image.image, &subres, &layout);

        VkResultSuccess(vkMapMemory(
            vulkan.device, image.memory, layout.offset, layout.size, 0, reinterpret_cast<void**>(&data)
        ));
    }

    MappedData::~MappedData()
    {
        if (!memory)
            return;

        vkUnmapMemory(vulkan.device, memory);
    }

    uint8_t*& MappedData::GetData()
    {
        return data;
    }
}
