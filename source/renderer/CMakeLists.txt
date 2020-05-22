ADD_LIBRARY(VulkanRenderer STATIC)

file(GLOB PublicHeaders "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

target_sources(VulkanRenderer
    PUBLIC
        ${PublicHeaders}
    PRIVATE
        Common.h
        Factory.cpp
        Utils.h
        Utils.cpp
        Texture.h
        Texture.cpp
        ScopeCommandBuffer.h
        ScopeCommandBuffer.cpp
        MappedData.h
        MappedData.cpp
        Buffer.h
        Buffer.cpp
        Shader.h
        Shader.cpp
        DescriptorSet.h
        DescriptorSet.cpp
        Pipeline.h
        Pipeline.cpp
        RenderPass.h
        RenderPass.cpp
        Camera.h
        Camera.cpp
)

target_include_directories(VulkanRenderer
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

source_group("Public" FILES ${PublicHeaders})

target_link_libraries(VulkanRenderer
    Vulkan::Vulkan
    Qt5::Gui
)

set_target_properties(VulkanRenderer PROPERTIES FOLDER Libraries)
