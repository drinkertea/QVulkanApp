#include <QVulkanWindow>
#include <QVulkanFunctions>

#include "VulkanFactory.h"
#include "VulkanUtils.h"

class VulkanFactory
    : public Vulkan::IVulkanFactory
{
    const QVulkanWindow&    m_window;
    VkDevice                m_device = nullptr;
    QVulkanFunctions&       m_vulkan_functions;
    QVulkanDeviceFunctions& m_device_functions;

    VkPhysicalDeviceMemoryProperties m_memory_properties;

    uint32_t GetMemoryIndex(uint32_t type_bits, VkMemoryPropertyFlags memory_property) const
    {
        for (uint32_t i = 0; i < m_memory_properties.memoryTypeCount; i++)
        {
            if ((type_bits & 1) == 1 && (m_memory_properties.memoryTypes[i].propertyFlags & memory_property) == memory_property)
                return i;
            type_bits >>= 1;
        }
        throw std::runtime_error("Could not find a matching memory type");
    }

    VkDeviceMemory Allocate(uint32_t index, VkDeviceSize size) const
    {
        VkMemoryAllocateInfo allocate_info = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            nullptr,
            size,
            index
        };

        VkDeviceMemory res = nullptr;
        Vulkan::VkResultSuccess(m_device_functions.vkAllocateMemory(m_device, &allocate_info, nullptr, &res));
        return res;
    }

public:
    VulkanFactory(const QVulkanWindow& window)
        : m_window(window)
        , m_device(m_window.device())
        , m_vulkan_functions(*m_window.vulkanInstance()->functions())
        , m_device_functions(*m_window.vulkanInstance()->deviceFunctions(m_device))
    {
        m_vulkan_functions.vkGetPhysicalDeviceMemoryProperties(m_window.physicalDevice(), &m_memory_properties);
    }

    ~VulkanFactory() override = default;

    Vulkan::Buffer CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memory_property, VkDeviceSize size, void* data) const override
    {
        Vulkan::Buffer buffer = {};
        buffer.size = size;

        VkBufferCreateInfo buffer_createInfo{};
        buffer_createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_createInfo.usage = usage;
        buffer_createInfo.size  = size;

        Vulkan::VkResultSuccess(m_device_functions.vkCreateBuffer(m_device, &buffer_createInfo, nullptr, &buffer.buffer));
        
        VkMemoryRequirements memory_requiments{};
        m_device_functions.vkGetBufferMemoryRequirements(m_device, buffer.buffer, &memory_requiments);

        buffer.memory = Allocate(GetMemoryIndex(memory_requiments.memoryTypeBits, memory_property), memory_requiments.size);

        //buffer->alignment = memory_requiments.alignment;
        //buffer->usageFlags = usageFlags;
        //buffer->memoryPropertyFlags = memoryPropertyFlags;

        if (data)
        {
            void* mapped = nullptr;
            Vulkan::VkResultSuccess(m_device_functions.vkMapMemory(m_device, buffer.memory, 0, size, 0, &mapped));
            memcpy(mapped, data, size);
            if ((memory_property & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange mapped_range = {};
                mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                mapped_range.memory = buffer.memory;
                mapped_range.offset = 0;
                mapped_range.size = size;
                Vulkan::VkResultSuccess(m_device_functions.vkFlushMappedMemoryRanges(m_device, 1, &mapped_range));
            }
            m_device_functions.vkUnmapMemory(m_device, buffer.memory);
        }

        Vulkan::VkResultSuccess(m_device_functions.vkBindBufferMemory(m_device, buffer.buffer, buffer.memory, 0));

        return buffer;
    }
};

std::unique_ptr<Vulkan::IVulkanFactory> Vulkan::IVulkanFactory::Create(const QVulkanWindow& window)
{
    return std::make_unique<VulkanFactory>(window);
}
