cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

set(CMAKE_AUTORCC ON)

set (CMAKE_CXX_STANDARD 17)

project(QVulkanApp)

include(cmake/qt-install.cmake)
get_qt(5.15.0)
set(CMAKE_PREFIX_PATH ${QT_DIR})
find_package(Qt5 COMPONENTS GUI REQUIRED)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(Vulkan REQUIRED)

include(cmake/googletest.cmake)

set(CMAKE_FOLDER external)
fetch_googletest(
    ${PROJECT_SOURCE_DIR}/cmake
    ${PROJECT_BINARY_DIR}/googletest
)

enable_testing()

add_subdirectory(source)
