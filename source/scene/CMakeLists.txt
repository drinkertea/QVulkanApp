ADD_LIBRARY(Scene STATIC
    include/IScene.h
    include/IResourceLoader.h

    Chunk.h
    Chunk.cpp
    Texture.h
    Texture.cpp
    Shader.h
    Shader.cpp
    Scene.cpp
)

target_include_directories(Scene PRIVATE ${PROJECT_SOURCE_DIR}/source/renderer/include)
target_include_directories(Scene PRIVATE ${PROJECT_SOURCE_DIR}/source/noiser)

target_link_libraries(Scene VulkanRenderer)
target_link_libraries(Scene NoiseGenerator)
