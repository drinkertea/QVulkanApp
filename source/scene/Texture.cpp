#include <DataProveder.h>
#include <IFactory.h>
#include <stdexcept>

#include "include/IResourceLoader.h"
#include "Texture.h"

namespace Scene
{

class TextureSource
    : public Vulkan::IDataProvider
{
    std::vector<uint8_t> data;
    uint32_t width  = 0u;
    uint32_t height = 1u;
    uint32_t depth  = 1u;

public:
    TextureSource(TextureType first, uint32_t count, const IResourceLoader& loader)
        : depth(count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            auto type_index = static_cast<uint32_t>(first) + i;
            if (type_index >= static_cast<uint32_t>(TextureType::Count))
                throw std::out_of_range("TextureType");

            loader.LoadTexture(static_cast<TextureType>(type_index), width, height, data);
        }
    }

    ~TextureSource() override = default;

    uint32_t GetWidth() const override
    {
        return width;
    }

    uint32_t GetHeight() const override
    {
        return height;
    }

    const uint8_t* GetData() const override
    {
        return data.data();
    }

    uint32_t GetSize() const override
    {
        return static_cast<uint32_t>(data.size());
    }

    uint32_t GetDepth() const override
    {
        return depth;
    }
};


Texture::Texture(TextureType first, uint32_t count, const IResourceLoader& loader, Vulkan::IFactory& factory)
    : texture(factory.CreateTexture(TextureSource(first, count, loader)))
    , offset(static_cast<uint32_t>(first))
{
}

uint32_t Texture::GetArrayIndex(TextureType type) const
{
    return offset + static_cast<uint32_t>(type);
}

const Vulkan::ITexture& Texture::GetTexture() const
{
    return texture;
}

}
