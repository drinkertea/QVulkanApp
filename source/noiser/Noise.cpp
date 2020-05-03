#include "FastNoise.h"
#include "Noise.h"

class Noise
    : public INoise
{
    FastNoise generator;
public:
    Noise(uint32_t seed)
        : generator(static_cast<int>(seed))
    {
    }

    ~Noise() override = default;

    int32_t GetHeight(int32_t x, int32_t y) const override
    {
        return static_cast<int32_t>(64.f * (1.f + generator.GetPerlin(
            static_cast<float>(x),
            static_cast<float>(y)
        )));
    }
};

std::unique_ptr<INoise> INoise::CreateNoise(uint32_t seed)
{
    return std::make_unique<Noise>(seed);
}

