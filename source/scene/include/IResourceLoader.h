#pragma once

#include <vector>

namespace Vulkan
{
enum class ShaderType;
}

namespace Scene
{

enum class TextureType : uint32_t
{
    Dirt = 0u,
    Stone,
    Sand,
    Bricks,
    Coblestone,
    GrassBlockTop,
    GrassBlockSide,
    Ice,
    Snow,
    OakPlanks,
    LeavesOakOpaque,
    OakLog,
    OakLogTop,
    WaterOverlay,
    Count,

    First = 0u
};

enum class ShaderTarget : uint32_t
{
    Block = 0u,
};

struct IResourceLoader
{
    virtual void LoadTexture(TextureType type, uint32_t& w, uint32_t& h, std::vector<uint8_t>& storage) const = 0;
    virtual std::vector<uint8_t> LoadShader(ShaderTarget target, Vulkan::ShaderType type) const = 0;

    virtual ~IResourceLoader() = default;
};

}
