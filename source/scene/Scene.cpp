#include <IFactory.h>
#include <ICamera.h>
#include <DataProvider.h>

#include <Noise.h>

#include "IScene.h"
#include "IResourceLoader.h"

#include "Chunk.h"
#include "Texture.h"
#include "Shader.h"

namespace Scene
{

enum class Corner : uint32_t
{
    TopLeft = 0,
    BotLeft,
    BotRight,
    TopRight,
};

static const std::vector<Corner> g_vertices = {
    Corner::TopLeft,
    Corner::BotLeft,
    Corner::BotRight,
    Corner::TopRight,
};

static const std::vector<uint32_t> g_indices = {
    0,1,2, 2,3,0,
};

static const Vulkan::Attributes g_vertex_attribs = { Vulkan::AttributeFormat::vec1i };

std::vector<Chunk> GetChunks(Vulkan::IFactory& factory, INoise& noiser)
{
    std::vector<Chunk> chunks;
    chunks.emplace_back(Point2D{ 0, 0 }, factory, noiser);
    return chunks;
}

class Scene
    : public IScene
{
    Vulkan::ICamera&                  camera;
    std::unique_ptr<Vulkan::IFactory> factory;
    std::unique_ptr<IResourceLoader>  loader;
    std::unique_ptr<INoise>           noiser;

    std::vector<Chunk> chunks;

    Texture textures;
    Program program;

    const Vulkan::IIndexBuffer&   index_buffer;
    const Vulkan::IVertexBuffer&  vertex_buffer;
    const Vulkan::IDescriptorSet& descriptor_set;
    const Vulkan::IPipeline&      pipeline;

    std::string info = "";

public:
    Scene(
        Vulkan::ICamera& camera,
        std::unique_ptr<Vulkan::IFactory> fac,
        std::unique_ptr<IResourceLoader> load
    )
        : camera(camera)
        , factory(std::move(fac))
        , loader(std::move(load))
        , noiser(INoise::CreateNoise(213312, 0.5f))
        , chunks(GetChunks(*factory, *noiser))
        , textures(TextureType::First, static_cast<uint32_t>(TextureType::Count), *loader, *factory)
        , program(ShaderTarget::Block, *loader, *factory)
        , vertex_buffer(factory->CreateVertexBuffer(Vulkan::BufferDataOwner<Corner>(g_vertices), g_vertex_attribs))
        , index_buffer(factory->CreateIndexBuffer(Vulkan::BufferDataOwner<uint32_t>(g_indices)))
        , descriptor_set(factory->CreateDescriptorSet(Vulkan::InputResources{ camera.GetMvpLayout(), textures.GetTexture() }))
        , pipeline(factory->CreatePipeline(descriptor_set, program.GetShaders(), vertex_buffer, chunks.front().GetData()))
    {
        camera.SetPosition(0.0f, 80.0f, 0.0f);
        camera.SetRotation(-20.0f, 180.0f, 0.0f);
    }

    ~Scene() override = default;

    void Render() override
    {
        auto render_pass = factory->CreateRenderPass(camera);
        render_pass->Bind(descriptor_set);
        render_pass->Bind(pipeline);
        render_pass->Bind(index_buffer, vertex_buffer);

        int draw_cnt = 0;
        for (const auto& chunk : chunks)
        {
            const auto& bbox = chunk.GetBBox();
            if (!camera.ObjectVisible(Vulkan::BBox{
                static_cast<float>(bbox.first.x),
                static_cast<float>(bbox.first.y),
                static_cast<float>(bbox.first.z),
                static_cast<float>(bbox.second.x),
                static_cast<float>(bbox.second.y),
                static_cast<float>(bbox.second.z),
            }))
                continue;

            ++draw_cnt;
            render_pass->Draw(chunk.GetData());
        }

        info = " - " + std::to_string(draw_cnt) + " chunks ";
    }

    const std::string& GetInfo() const override
    {
        return info;
    }
};

std::unique_ptr<IScene> IScene::Create(
    Vulkan::ICamera& camera,
    std::unique_ptr<Vulkan::IFactory> factory,
    std::unique_ptr<IResourceLoader> loader
)
{
    return std::make_unique<Scene>(camera, std::move(factory), std::move(loader));
}

}
