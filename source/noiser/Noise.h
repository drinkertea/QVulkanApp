#pragma once

#include <cstdint>
#include <memory>

struct INoise
{
    virtual ~INoise() = default;
    virtual uint32_t GetHeight(uint32_t x, uint32_t y) const = 0;

    static std::unique_ptr<INoise> CreateNoise(uint32_t seed);
};

