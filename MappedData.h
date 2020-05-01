#pragma once

#include <vulkan/vulkan.h>

#include "VulkanFactory.h"

class QVulkanWindow;
struct Image;

namespace Vulkan
{

class MappedData
    : public IMappedData
{
public:
    MappedData(const Image& image, const QVulkanWindow& window);
    ~MappedData() override;

    const bytes& GetData() const override;

private:
    const QVulkanWindow& m_window;

    VkDeviceMemory memory = nullptr;
    bytes          data = nullptr;
};

}
