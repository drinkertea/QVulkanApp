#pragma once

#include "IFactory.h"
#include <vector>

namespace Scene
{
enum class ShaderTarget : uint32_t;
struct IResourceLoader;

class Program
{
public:
    Program(ShaderTarget target, const IResourceLoader& loader, Vulkan::IFactory& factory);
    ~Program() = default;

    const Vulkan::Shaders& GetShaders() const;

private:
    Vulkan::Shaders shaders;
};

}
