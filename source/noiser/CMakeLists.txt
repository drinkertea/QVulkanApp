ADD_LIBRARY(NoiseGenerator STATIC)

target_sources(NoiseGenerator
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Noise.h
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/Noise.cpp
        ${PROJECT_SOURCE_DIR}/external/FastNoise/FastNoise.h
        ${PROJECT_SOURCE_DIR}/external/FastNoise/FastNoise.cpp
)

target_include_directories(NoiseGenerator
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/external/FastNoise
)

set_target_properties(NoiseGenerator PROPERTIES FOLDER Libraries)
