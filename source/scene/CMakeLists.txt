file(GLOB PublicHeaders
    include/IScene.h
    include/IResourceLoader.h
)
source_group("Public" FILES ${PublicHeaders})

ADD_LIBRARY(Scene STATIC
    ${PublicHeaders}
    Chunk.h
    Chunk.cpp
    Texture.h
    Texture.cpp
    Shader.h
    Shader.cpp
    Scene.cpp
    ChunkUtils.h
    ChunkUtils.cpp
)

target_include_directories(Scene PRIVATE include)

target_link_libraries(Scene VulkanRenderer)
target_include_directories(Scene PRIVATE ${PROJECT_SOURCE_DIR}/source/renderer/include)

target_link_libraries(Scene NoiseGenerator)
target_include_directories(Scene PRIVATE ${PROJECT_SOURCE_DIR}/source/noiser)
