cmake_minimum_required(VERSION 3.1.0)

set (CMAKE_PREFIX_PATH "C:\\Qt\\5.14.2\\msvc2017_64")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(QVulkanApp)

#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

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

add_subdirectory(source/renderer)
add_subdirectory(source/noiser)
add_subdirectory(source/scene)
add_subdirectory(source/application)
