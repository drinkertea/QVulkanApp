cmake_minimum_required(VERSION 3.1.0)

find_package(Vulkan REQUIRED)
find_package(Qt5 COMPONENTS GUI REQUIRED)

file(GLOB PublicHeaders
    include/VulkanUtils.h
    include/VulkanFactory.h
    include/IRenderer.h
)
source_group("Public" FILES ${PublicHeaders})

ADD_LIBRARY(VulkanRenderer STATIC
    ${PublicHeaders}
    VulkanUtils.cpp
    private/Factory.h
    private/Factory.cpp
    private/Texture.h
    private/Texture.cpp
    private/ScopeCommandBuffer.h
    private/ScopeCommandBuffer.cpp
    private/MappedData.h
    private/MappedData.cpp
)

target_include_directories(VulkanRenderer PRIVATE include)

target_link_libraries(VulkanRenderer Vulkan::Vulkan)
target_include_directories(VulkanRenderer PRIVATE Vulkan::Vulkan)
target_compile_definitions(VulkanRenderer PRIVATE VK_USE_PLATFORM_WIN32_KHR)

target_link_libraries(VulkanRenderer Qt5::Gui)