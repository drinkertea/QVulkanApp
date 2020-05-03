cmake_minimum_required(VERSION 3.1.0)

ADD_LIBRARY(NoiseGenerator STATIC
    Noise.h
    Noise.cpp

    ${PROJECT_SOURCE_DIR}/external/FastNoise/FastNoise.h
    ${PROJECT_SOURCE_DIR}/external/FastNoise/FastNoise.cpp
)

target_include_directories(NoiseGenerator PRIVATE ${PROJECT_SOURCE_DIR}/external/FastNoise)
