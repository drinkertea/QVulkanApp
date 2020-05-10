#include <IFactory.h>
#include <ICamera.h>
#include <DataProvider.h>

#include <Noise.h>

#include "IScene.h"
#include "IResourceLoader.h"

#include "Chunk.h"
#include "Texture.h"
#include "Shader.h"

#include <mutex>

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

class ChunkStorage
{
    Vulkan::ICamera& camera;
    Vulkan::IFactory& factory;

    std::unique_ptr<INoise> noiser;

    static constexpr int32_t render_distance = 16;

    std::thread generator_thread;
    std::mutex chunks_mutex;
    std::vector<std::unique_ptr<Chunk>> chunks;

    CreationPool creation_pool;

public:
    void PushChunk(const Point2D& pos)
    {
        auto chunk = std::make_unique<Chunk>(pos, factory, *noiser, creation_pool);
        std::lock_guard<std::mutex> lg(chunks_mutex);
        chunks.emplace_back(std::move(chunk));
    }

    void GenerateProc()
    {
        auto mid = GetCamPos();
        while (chunks.empty());

        for (int dist = 1; dist < render_distance; ++dist)
        {
            for (int i = -dist; i < dist; ++i)
                PushChunk({ mid.x - dist, mid.y + i });
            for (int i = -dist; i < dist; ++i)
                PushChunk({ mid.x + i, mid.y + dist });

            for (int i = dist; i > -dist; --i)
                PushChunk({ mid.x + dist, mid.y + i });
            for (int i = dist; i > -dist; --i)
                PushChunk({ mid.x + i, mid.y - dist });
        }
    }

    Point2D GetCamPos() const
    {
        auto pos = camera.GetViewPos();
        return { static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.z) };
    }

    ChunkStorage(Vulkan::ICamera& camera, Vulkan::IFactory& factory)
        : camera(camera)
        , factory(factory)
        , noiser(INoise::CreateNoise(213312, 0.5f))
        , generator_thread(&ChunkStorage::GenerateProc, this)
    {
        PushChunk(GetCamPos());
        CreateGpuChunks();
    }

    ~ChunkStorage()
    {
        if (generator_thread.joinable())
            generator_thread.join();
    }

    const Vulkan::IInstanceBuffer& GetInstanceLayout()
    {
        return chunks.front()->GetData();
    }

    void CreateGpuChunks()
    {
        size_t size = creation_pool.size();
        while (size--)
        {
            creation_pool.front()();
            creation_pool.pop_front();
        }
    }

    void ForEach(const std::function<void(const Chunk&)>& callback)
    {
        std::lock_guard<std::mutex> lg(chunks_mutex);
        for (const auto& chunk : chunks)
        {
            if (!*chunk)
                continue;

            callback(*chunk);
        }
    }
};

class Scene
    : public IScene
{
    Vulkan::ICamera&                  camera;
    std::unique_ptr<Vulkan::IFactory> factory;
    std::unique_ptr<IResourceLoader>  loader;

    ChunkStorage chunk_storage;
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
        , chunk_storage(camera, *factory)
        , textures(TextureType::First, static_cast<uint32_t>(TextureType::Count), *loader, *factory)
        , program(ShaderTarget::Block, *loader, *factory)
        , vertex_buffer(factory->CreateVertexBuffer(Vulkan::BufferDataOwner<Corner>(g_vertices), g_vertex_attribs))
        , index_buffer(factory->CreateIndexBuffer(Vulkan::BufferDataOwner<uint32_t>(g_indices)))
        , descriptor_set(factory->CreateDescriptorSet(Vulkan::InputResources{ camera.GetMvpLayout(), textures.GetTexture() }))
        , pipeline(factory->CreatePipeline(descriptor_set, program.GetShaders(), vertex_buffer, chunk_storage.GetInstanceLayout()))
    {
        camera.SetPosition(0.0f, 80.0f, 0.0f);
        camera.SetRotation(-20.0f, 180.0f, 0.0f);
    }

    ~Scene() override = default;

    void Render() override
    {
        chunk_storage.CreateGpuChunks();

        auto render_pass = factory->CreateRenderPass(camera);
        render_pass->Bind(descriptor_set);
        render_pass->Bind(pipeline);
        render_pass->Bind(index_buffer, vertex_buffer);

        int draw_cnt = 0;
        chunk_storage.ForEach([&](const Chunk& chunk)
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

std::unique_ptr<IScene> IScene::Create(
    Vulkan::ICamera& camera,
    std::unique_ptr<Vulkan::IFactory> factory,
    std::unique_ptr<IResourceLoader> loader
)
{
    return std::make_unique<Scene>(camera, std::move(factory), std::move(loader));
}

}
