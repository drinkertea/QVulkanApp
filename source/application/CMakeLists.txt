cmake_minimum_required(VERSION 3.1.0)

add_executable(QVulkanApp
    main.cpp
    RenderInterface.h
    RenderInterface.cpp
    Loader.cpp
    ${PROJECT_SOURCE_DIR}/textures/textures.qrc
    ${PROJECT_SOURCE_DIR}/shaders/shaders.qrc

    ${SHADERS}
)

target_link_libraries(QVulkanApp VulkanRenderer)
target_include_directories(QVulkanApp PRIVATE ${PROJECT_SOURCE_DIR}/source/renderer/include)

target_link_libraries(QVulkanApp Scene)
target_include_directories(QVulkanApp PRIVATE ${PROJECT_SOURCE_DIR}/source/scene/include)

target_link_libraries(QVulkanApp Vulkan::Vulkan)
target_include_directories(QVulkanApp PRIVATE Vulkan::Vulkan)
target_compile_definitions(QVulkanApp PRIVATE VK_USE_PLATFORM_WIN32_KHR)

target_link_libraries(QVulkanApp Qt5::Gui)