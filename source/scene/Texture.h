#pragma once

#include "IFactory.h"

#include <vector>
#include <string>

namespace Scene
{

enum class TextureType : uint32_t
{
    Dirt = 0,
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
    Count,

    First = 0
};

constexpr uint32_t texture_type_count = static_cast<uint32_t>(TextureType::Count);

struct ITextureLoader
{
    virtual void LoadTexture(TextureType type, uint32_t& w, uint32_t& h, std::vector<uint8_t>& storage) const = 0;
    virtual ~ITextureLoader() = default;
};

class Texture
{
public:
    Texture(TextureType first, uint32_t count, const ITextureLoader& loader, Vulkan::IFactory& factory);
    ~Texture() = default;

    uint32_t GetArrayIndex(TextureType type) const;
    const Vulkan::ITexture& GetTexture() const;

private:
    const Vulkan::ITexture& texture;

    uint32_t offset = 0u;
};

}
