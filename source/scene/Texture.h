#pragma once

#include <vector>
#include <string>

namespace Vulkan
{
struct IFactory;
struct ITexture;
}

namespace Scene
{

enum class TextureType : uint32_t;
struct IResourceLoader;

class Texture
{
public:
    Texture(TextureType first, uint32_t count, const IResourceLoader& loader, Vulkan::IFactory& factory);
    ~Texture() = default;

    uint32_t GetArrayIndex(TextureType type) const;
    const Vulkan::ITexture& GetTexture() const;

private:
    const Vulkan::ITexture& texture;

    uint32_t offset = 0u;
};

}
