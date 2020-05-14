#include <IFactory.h>
#include <ICamera.h>
#include <DataProvider.h>

#include "IScene.h"
#include "IResourceLoader.h"

#include "Chunk.h"
#include "ChunkStorage.h"
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

constexpr uint32_t g_texture_type_count = static_cast<uint32_t>(TextureType::Count);

class Scene : public IScene
{
    Vulkan::ICamera&                  camera;
    std::unique_ptr<Vulkan::IFactory> factory;
    std::unique_ptr<IResourceLoader>  loader;
    std::unique_ptr<IChunkStorage>    chunk_storage;

    Texture textures;
    Program solid_block_program;

    const Vulkan::IIndexBuffer&   index_buffer;
    const Vulkan::IVertexBuffer&  vertex_buffer;
    const Vulkan::IDescriptorSet& descriptor_set;
    const Vulkan::IPipeline&      pipeline;

    std::string info = "";

public:
    Scene(Vulkan::ICamera& camera, std::unique_ptr<Vulkan::IFactory> fac, std::unique_ptr<IResourceLoader> load)
        : camera(camera)
        , factory(std::move(fac))
        , loader(std::move(load))
        , chunk_storage(IChunkStorage::Create(camera, *factory))
        , textures(TextureType::First, g_texture_type_count, *loader, *factory)
        , solid_block_program(ShaderTarget::Block, *loader, *factory)
        , vertex_buffer (factory->CreateVertexBuffer(Vulkan::BufferDataOwner<Corner>(g_vertices), g_vertex_attribs))
        , index_buffer  (factory->CreateIndexBuffer(Vulkan::BufferDataOwner<uint32_t>(g_indices)))
        , descriptor_set(factory->CreateDescriptorSet(Vulkan::InputResources{ camera.GetMvpLayout(), textures.GetTexture() }))
        , pipeline      (factory->CreatePipeline(descriptor_set, solid_block_program.GetShaders(), vertex_buffer, chunk_storage->GetInstanceLayout()))
    {
    }

    ~Scene() override = default;

    void Render() override
    {
        auto render_pass = factory->CreateRenderPass(camera);
        chunk_storage->OnRender();

        render_pass->Bind(descriptor_set);
        render_pass->Bind(pipeline);
        render_pass->Bind(index_buffer, vertex_buffer);

        int draw_cnt = 0;
        chunk_storage->ForEach([&](const Chunk& chunk)
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
                return;

            ++draw_cnt;
            render_pass->Draw(chunk.GetData());
        });

        info = " - " + std::to_string(draw_cnt) + " chunks ";
    }

    const std::string& GetInfo() const override
    {
        return info;
    }
};

std::unique_ptr<IScene> IScene::Create(Vulkan::ICamera& camera, std::unique_ptr<Vulkan::IFactory> factory, std::unique_ptr<IResourceLoader> loader)
{
    return std::make_unique<Scene>(camera, std::move(factory), std::move(loader));
}

}
