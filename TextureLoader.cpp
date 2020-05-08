#include <QImage>

#include "Texture.h"

using Scene::TextureType;

class TextureLoader
    : public Scene::ITextureLoader
{
    static bool ShouldColorize(TextureType type)
    {
        switch (type)
        {
        case Scene::TextureType::GrassBlockTop:
        case Scene::TextureType::LeavesOakOpaque:
            return true;
        default:
            break;
        }
        return false;
    }

    static QString GetTexturePath(TextureType type)
    {
        switch (type)
        {
        case Scene::TextureType::Dirt:            return ":/dirt.png";
        case Scene::TextureType::Stone:           return ":/stone.png"; 
        case Scene::TextureType::Sand:            return ":/sand.png"; 
        case Scene::TextureType::Bricks:          return ":/bricks.png"; 
        case Scene::TextureType::Coblestone:      return ":/cobblestone.png";
        case Scene::TextureType::GrassBlockTop:   return ":/grass_block_top.png"; 
        case Scene::TextureType::GrassBlockSide:  return ":/grass_block_side.png";
        case Scene::TextureType::Ice:             return ":/ice.png"; 
        case Scene::TextureType::Snow:            return ":/snow.png";
        case Scene::TextureType::OakPlanks:       return ":/oak_planks.png";
        case Scene::TextureType::LeavesOakOpaque: return ":/leaves_oak_opaque.png";
        case Scene::TextureType::OakLog:          return ":/oak_log.png";
        case Scene::TextureType::OakLogTop:       return ":/oak_log_top.png";
        default:
            throw std::logic_error("Unhandled enum value");
        }
    }

    void LoadTexture(TextureType type, uint32_t& w, uint32_t& h, std::vector<uint8_t>& storage) const override
    {
        auto url = GetTexturePath(type);
        QImage image(url);
        if (image.isNull())
            throw std::runtime_error("Cannot find the path specified: " + url.toStdString());

        image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

        if (ShouldColorize(type))
        {
            for (int x = 0; x < image.width(); ++x)
            {
                for (int y = 0; y < image.width(); ++y)
                {
                    auto color = image.pixelColor(x, y);
                    color.setRed(47.f * 3.1f * color.redF());
                    color.setGreen(75.f * 3.1f * color.greenF());
                    color.setBlue(35.f * 3.1f * color.blueF());
                    image.setPixelColor(x, y, color);
                }
            }
        }

        w = image.width();
        h = image.height();

        auto curr_pos = storage.size();
        storage.resize(curr_pos + static_cast<size_t>(w * h * 4u));
        auto mapped_data = &storage[curr_pos];

        for (uint32_t y = 0; y < h; ++y)
        {
            auto row_pitch = w * 4; // todo: provide format or row pith
            memcpy(mapped_data, image.constScanLine(y), row_pitch);
            mapped_data += row_pitch;
        }
    }
};

std::unique_ptr<Scene::ITextureLoader> CreateTextureLoader()
{
    return std::make_unique<TextureLoader>();
}