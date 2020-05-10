#pragma once

#include "IFactory.h"
#include <vector>

namespace Scene
{

enum class ShaderTarget : uint32_t
{
    Block = 0,
};

struct IShaderLoader
{
    virtual std::vector<uint8_t> LoadShader(ShaderTarget target, Vulkan::ShaderType type) const = 0;
    virtual ~IShaderLoader() = default;
};

class Program
{
public:
    Program(ShaderTarget target, const IShaderLoader& loader, Vulkan::IFactory& factory);
    ~Program() = default;

    const Vulkan::Shaders& GetShaders() const;

private:
    Vulkan::Shaders shaders;
};

}
