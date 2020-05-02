cmake_minimum_required(VERSION 3.1.0)

find_package(Vulkan REQUIRED)

ADD_LIBRARY(VulkanRenderer STATIC
    include/VulkanUtils.h
    include/VulkanImage.h
    include/VulkanFactory.h
    VulkanImage.cpp
    VulkanUtils.cpp
)

target_include_directories(VulkanRenderer PRIVATE include)

target_compile_definitions(VulkanRenderer PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(VulkanRenderer PRIVATE Vulkan::Vulkan)
target_link_libraries(VulkanRenderer Vulkan::Vulkan)