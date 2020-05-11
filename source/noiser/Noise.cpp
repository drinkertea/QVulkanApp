#include <FastNoise.h>
#include "Noise.h"

class Noise
    : public INoise
{
    FastNoise generator01;
    FastNoise generator02;
    FastNoise generator04;

    float amplitude = 0.f;

public:
    Noise(uint32_t seed, float amplitude)
        : generator01(static_cast<int>(seed))
        , generator02(static_cast<int>(seed))
        , generator04(static_cast<int>(seed))
        , amplitude(amplitude)
    {
        generator01.SetFrequency(0.01f);
        generator02.SetFrequency(0.02f);
        generator04.SetFrequency(0.04f);
    }

    ~Noise() override = default;

    static float GetHeight(const FastNoise& gen, int32_t x, int32_t y, float amplitude)
    {
        return amplitude * gen.GetPerlin(
            static_cast<float>(x),
            static_cast<float>(y)
        );
    }

    int32_t GetHeight(int32_t x, int32_t y) const override
    {
        return static_cast<int32_t>(64.f * (1.f + 
            GetHeight(generator01, x, y, amplitude) +
            GetHeight(generator02, x, y, amplitude / 2) +
            GetHeight(generator04, x, y, amplitude / 4)
        ));
    }
};

std::unique_ptr<INoise> INoise::CreateNoise(uint32_t seed, float amplitude)
{
    return std::make_unique<Noise>(seed, amplitude);
}
