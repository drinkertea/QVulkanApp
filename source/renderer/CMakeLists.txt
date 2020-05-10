cmake_minimum_required(VERSION 3.1.0)

file(GLOB PublicHeaders
    include/IFactory.h
    include/IRenderer.h
    include/ICamera.h
    include/DataProvider.h
)
source_group("Public" FILES ${PublicHeaders})

ADD_LIBRARY(VulkanRenderer STATIC
    ${PublicHeaders}
    private/Factory.cpp
    private/Utils.h
    private/Utils.cpp
    private/Texture.h
    private/Texture.cpp
    private/ScopeCommandBuffer.h
    private/ScopeCommandBuffer.cpp
    private/MappedData.h
    private/MappedData.cpp
    private/Buffer.h
    private/Buffer.cpp
    private/Shader.h
    private/Shader.cpp
    private/DescriptorSet.h
    private/DescriptorSet.cpp
    private/Pipeline.h
    private/Pipeline.cpp
    private/RenderPass.h
    private/RenderPass.cpp
    private/Camera.h
    private/Camera.cpp
)

target_include_directories(VulkanRenderer PRIVATE include)

target_link_libraries(VulkanRenderer Vulkan::Vulkan)
target_include_directories(VulkanRenderer PRIVATE Vulkan::Vulkan)
target_compile_definitions(VulkanRenderer PRIVATE VK_USE_PLATFORM_WIN32_KHR)

target_link_libraries(VulkanRenderer Qt5::Gui)