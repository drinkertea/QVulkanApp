#include <QVulkanWindow>
#include <QVulkanFunctions>

#include "MappedData.h"
#include "Texture.h"
#include "Utils.h"

namespace Vulkan
{
    MappedData::MappedData(const Image& image, const QVulkanWindow& window)
        : m_window(window)
        , memory(image.memory)
    {
        auto& device_functions = *window.vulkanInstance()->deviceFunctions(window.device());

        VkImageSubresource subres = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
        VkSubresourceLayout layout;
        device_functions.vkGetImageSubresourceLayout(window.device(), image.image, &subres, &layout);

        VkResultSuccess(device_functions.vkMapMemory(
            window.device(), image.memory, layout.offset, layout.size, 0, reinterpret_cast<void**>(&data)
        ));
    }

    MappedData::~MappedData()
    {
        if (!memory)
            return;

        m_window.vulkanInstance()->deviceFunctions(m_window.device())->vkUnmapMemory(m_window.device(), memory);
    }

    uint8_t*& MappedData::GetData()
    {
        return data;
    }
}
