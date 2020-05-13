ADD_LIBRARY(Scene STATIC)

file(GLOB PublicHeaders "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

target_sources(Scene
    PUBLIC
        ${PublicHeaders}
    PRIVATE
        Chunk.h
        Chunk.cpp
        Texture.h
        Texture.cpp
        Shader.h
        Shader.cpp
        Scene.cpp
        ChunkUtils.h
        ChunkUtils.cpp
        ThreadUtils.hpp
)

target_include_directories(Scene
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)
source_group("Public" FILES ${PublicHeaders})

target_link_libraries(Scene
    VulkanRenderer
    NoiseGenerator
)

set_target_properties(Scene PROPERTIES FOLDER Libraries)

add_subdirectory(tests)
