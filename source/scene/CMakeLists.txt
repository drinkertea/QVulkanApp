ADD_LIBRARY(Scene STATIC
    Chunk.h
    Chunk.cpp
)

target_include_directories(Scene PRIVATE ${PROJECT_SOURCE_DIR}/source/renderer/include)
target_include_directories(Scene PRIVATE ${PROJECT_SOURCE_DIR}/source/noiser)

target_link_libraries(Scene VulkanRenderer)
target_link_libraries(Scene NoiseGenerator)
