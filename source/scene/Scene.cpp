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
#include <map>
#include <set>
#include <atomic>
#include <condition_variable>

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

static const Point2D g_invalid_pos = { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min() };

struct ChunkKey
{
    int32_t d = 0;
    int32_t o = 0;

    ChunkKey(const Point2D& mid, const Point2D& pos)
    {
        int32_t x = pos.x - mid.x;
        int32_t y = pos.y - mid.y;
        d = std::max(std::abs(x), std::abs(y));

        if (x == d)
            o = d - y;
        else if (y == -d)
            o = d * 3 - x;
        else if (x == -d)
            o = d * 5 + y;
        else
            o = d * 7 + x;
    }

    bool operator<(const ChunkKey& r) const
    {
        return std::tie(d, o) < std::tie(r.d, r.o);
    }
};

class ContiniousPool
{
    std::thread worker;
    std::atomic_bool terminated = false;
    std::atomic_bool want_add = false;

    std::map<ChunkKey, std::function<void()>> pool;
    std::mutex pool_mutex;
    std::condition_variable add_cv;

    void ExecutionThread()
    {
        std::unique_lock<std::mutex> lg(pool_mutex);
        while (!terminated)
        {
            while (want_add)
                add_cv.wait(lg);

            if (pool.empty())
            {
                std::this_thread::sleep_for(std::chrono::microseconds(25u));
                continue;
            }

            auto task = pool.begin();
            task->second();
            pool.erase(task);
        }
    }

public:
    ContiniousPool()
        : worker(&ContiniousPool::ExecutionThread, this)
    {
    }

    void AddTask(const ChunkKey& key, std::function<void()>&& task)
    {
        want_add = true;
        std::lock_guard<std::mutex> lg(pool_mutex);
        pool[key].swap(std::move(task));
        want_add = false;
        add_cv.notify_all();
    }

    ~ContiniousPool()
    {
        terminated = true;
        while (!worker.joinable());
        worker.join();
    }
};

class ChunkStorage
{
    Vulkan::ICamera& camera;
    Vulkan::IFactory& factory;
    TaskQueue gpu_creation_pool;

    std::unique_ptr<INoise> noiser;

    static constexpr int32_t render_distance = 4;

    using Chunks = std::map<ChunkKey, std::unique_ptr<Chunk>>;
    Chunks chunks;
    std::mutex chunks_mutex;

    Point2D current_chunk = g_invalid_pos;

    ContiniousPool cpu_creation_pool;

public:
    static std::vector<Point2D> GetRenderScope(const Point2D& mid)
    {
        std::vector<Point2D> res;
        res.push_back(mid);
        for (int dist = 1; dist < render_distance; ++dist)
        {
            for (int i = dist; i > -dist; --i)
                res.push_back(Point2D{ mid.x + dist, mid.y + i });
            for (int i = dist; i > -dist; --i)
                res.push_back(Point2D{ mid.x + i, mid.y - dist });
            for (int i = -dist; i < dist; ++i)
                res.push_back(Point2D{ mid.x - dist, mid.y + i });
            for (int i = -dist; i < dist; ++i)
                res.push_back(Point2D{ mid.x + i, mid.y + dist });
        }
        return res;
    }

    Point2D GetCamPos() const
    {
        auto pos = camera.GetViewPos();
        return Chunk::GetChunkBase({ static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.z) });
    }

    ChunkStorage(Vulkan::ICamera& camera, Vulkan::IFactory& factory)
        : camera(camera)
        , factory(factory)
        , noiser(INoise::CreateNoise(213312, 0.5f))
    {
        AddCpuTasks();
        while (chunks.empty())
            std::this_thread::sleep_for(std::chrono::microseconds(1u));
        RenderBegin();
    }

    const Vulkan::IInstanceBuffer& GetInstanceLayout()
    {
        while (chunks.empty() || !*chunks.begin()->second)
            std::this_thread::sleep_for(std::chrono::microseconds(1u));
        return chunks.begin()->second->GetData();
    }

    void AddCpuTasks()
    {
        auto cam_chunk = GetCamPos();
        if (cam_chunk == current_chunk)
            return;

        Point2D translation = { cam_chunk.x - current_chunk.x, cam_chunk.y - current_chunk.y };
        current_chunk = cam_chunk;
        auto to_render = GetRenderScope(current_chunk);

        for (const auto& pos : to_render)
        {
            ChunkKey key(current_chunk, pos);
            cpu_creation_pool.AddTask(key, std::bind([this](const ChunkKey& key, const Point2D& pos) {
                auto chunk = std::make_unique<Chunk>(pos, factory, *noiser, gpu_creation_pool);
                std::lock_guard<std::mutex> lg(chunks_mutex);
                chunks[key].swap(chunk);
            }, key, pos));
        }
    }

    void RenderBegin()
    {
        gpu_creation_pool.ExecuteAll();
    }

    void ForEach(const std::function<void(const Chunk&)>& callback)
    {
        std::lock_guard<std::mutex> lg(chunks_mutex);
        for (const auto& chunk : chunks)
        {
            if (!*chunk.second)
                continue;

            callback(*chunk.second);
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
        chunk_storage.AddCpuTasks();
        chunk_storage.RenderBegin();
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
