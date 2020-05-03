#pragma once

#include <cstdint>
#include <memory>

struct INoise
{
    virtual ~INoise() = default;
    virtual int32_t GetHeight(int32_t x, int32_t y) const = 0;

    static std::unique_ptr<INoise> CreateNoise(uint32_t seed);
};

