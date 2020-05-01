#pragma once

#include <vulkan/vulkan.h>

#include "VulkanFactory.h"

class QVulkanWindow;

namespace Vulkan
{

struct Image;

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
