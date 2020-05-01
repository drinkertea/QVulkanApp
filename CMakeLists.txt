cmake_minimum_required(VERSION 3.1.0)

set (CMAKE_PREFIX_PATH "C:\\Qt\\5.14.2\\msvc2017_64")

project(QVulkanApp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Vulkan REQUIRED)
find_package(Qt5 COMPONENTS GUI REQUIRED)

set(SHADER_DIR "shaders")
file(GLOB SHADERS
    "${SHADER_DIR}/*.vert"
    "${SHADER_DIR}/*.frag"
    "${SHADER_DIR}/*.comp"
    "${SHADER_DIR}/*.geom"
    "${SHADER_DIR}/*.tesc"
    "${SHADER_DIR}/*.tese"
    "${SHADER_DIR}/*.mesh"
    "${SHADER_DIR}/*.task"
    "${SHADER_DIR}/*.rgen"
    "${SHADER_DIR}/*.rchit"
    "${SHADER_DIR}/*.rmiss"
)
source_group("Shaders" FILES ${SHADERS})

add_executable(QVulkanApp
    main.cpp
    Camera.h
    RenderInterface.h
    RenderInterface.cpp
    VulkanFactory.h
    VulkanFactory.cpp
    textures/textures.qrc
    shaders/shaders.qrc

    ${SHADERS}
)

target_compile_definitions(QVulkanApp PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(QVulkanApp PRIVATE Vulkan::Vulkan)
target_link_libraries(QVulkanApp Vulkan::Vulkan)

target_link_libraries(QVulkanApp Qt5::Gui)