cmake_minimum_required(VERSION 3.1.0)

set (CMAKE_PREFIX_PATH "C:\\Qt\\5.14.2\\msvc2017_64")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(QVulkanApp)

set(CMAKE_AUTORCC ON)

find_package(Vulkan REQUIRED)
find_package(Qt5 COMPONENTS GUI REQUIRED)

add_subdirectory(source)
