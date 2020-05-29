#include <IFactory.h>
#include <ICamera.h>
#include <DataProvider.h>

#include "IScene.h"
#include "IResourceLoader.h"

#include "Chunk.h"
#include "ChunkStorage.h"
#include "Texture.h"
#include "Shader.h"
#include "ThreadUtils.hpp"

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

static const Vulkan::Attributes g_vertex_attribs = {
    Vulkan::AttributeFormat::vec1i
};

static const Vulkan::Attributes g_instance_attributes = {
    Vulkan::AttributeFormat::vec3f,
    Vulkan::AttributeFormat::vec1i,
    Vulkan::AttributeFormat::vec1i,
};

constexpr uint32_t g_texture_type_count = static_cast<uint32_t>(TextureType::Count);

class Scene : public IScene
{
    Vulkan::ICamera&                  camera;
    std::unique_ptr<Vulkan::IFactory> factory;
    std::unique_ptr<IResourceLoader>  loader;
    std::unique_ptr<IChunkStorage>    chunk_storage;

    Texture textures;
    Program solid_block_program;

    const Vulkan::IVertexLayout& vertex_layout = [](Vulkan::IFactory& factory) {
        std::reference_wrapper<Vulkan::IVertexLayout> res = factory.AddVertexLayout();
        auto& vertex = res.get().AddVertexBinding();
        for (auto& attrib : g_vertex_attribs)
            vertex.AddAttribute(attrib);
        auto& instance = res.get().AddInstanceBinding();
        for (auto& attrib : g_instance_attributes)
            instance.AddAttribute(attrib);
        return res;
    }(*factory);

    const Vulkan::IBuffer&        index_buffer;
    const Vulkan::IBuffer&        vertex_buffer;
    const Vulkan::IDescriptorSet& descriptor_set;
    const Vulkan::IPipeline&      pipeline;

    uint32_t thread_count = 1;// std::thread::hardware_concurrency();
    std::vector<utils::SimpleThread::Ptr> draw_threads = [](uint32_t thread_count)
    {
        std::vector<utils::SimpleThread::Ptr> draw_threads;
        for (uint32_t i = 0; i < thread_count; ++i)
            draw_threads.emplace_back(std::make_unique<utils::SimpleThread>());
        return draw_threads;
    }(thread_count);

    using CommandBufferRef = std::reference_wrapper<Vulkan::ICommandBuffer>;
    std::vector<CommandBufferRef> command_buffers = [](Vulkan::IFactory& factory, Vulkan::ICamera& camera, uint32_t thread_count)
    {
        std::vector<CommandBufferRef> command_buffers;
        for (uint32_t i = 0; i < thread_count; ++i)
            command_buffers.emplace_back(factory.AddCommandBuffer(camera));
        return command_buffers;
    }(*factory, camera, thread_count);

    std::string info = "";

public:
    Scene(Vulkan::ICamera& camera, std::unique_ptr<Vulkan::IFactory> fac, std::unique_ptr<IResourceLoader> load)
        : camera(camera)
        , factory(std::move(fac))
        , loader(std::move(load))
        , chunk_storage(IChunkStorage::Create(camera, *factory))
        , textures(TextureType::First, g_texture_type_count, *loader, *factory)
        , solid_block_program(ShaderTarget::Block, *loader, *factory)
        , vertex_buffer (factory->AddBuffer(Vulkan::BufferUsage::Vertex, Vulkan::BufferDataOwner<Corner>(g_vertices)))
        , index_buffer  (factory->AddBuffer(Vulkan::BufferUsage::Index, Vulkan::BufferDataOwner<uint32_t>(g_indices)))
        , descriptor_set(factory->CreateDescriptorSet(Vulkan::InputResources{ camera.GetMvpLayout(), textures.GetTexture() }))
        , pipeline      (factory->CreatePipeline(descriptor_set, solid_block_program.GetShaders(), vertex_layout))
    {
    }

    ~Scene() override = default;

    void Render() override
    {
        std::atomic_uint32_t draw_cnt = 0;
        std::vector<std::reference_wrapper<const Chunk>> frustrum_passed_water_chunks;
        std::mutex water_mutex;

        chunk_storage->OnRender();

        {
            auto render_pass = factory->CreateRenderPass(camera);

            for (uint32_t i = 0; i < thread_count; ++i)
            {
                auto& command_buffer = command_buffers.at(i).get();
                render_pass->AddCommandBuffer(command_buffer);

                command_buffer.Bind(descriptor_set);
                command_buffer.Bind(pipeline);
                command_buffer.Bind(index_buffer);
                command_buffer.Bind(vertex_buffer);
            }

            size_t thread_index = 0u;
            chunk_storage->ForEach([&](const Chunk& chunk)
            {
                draw_threads[thread_index++ % thread_count]->Add(std::bind([&](size_t thread_index) {
                    const auto& bbox = chunk.GetBBox();
                    //if (!camera.ObjectVisible(Vulkan::BBox{
                    //    static_cast<float>(bbox.first.x),
                    //    static_cast<float>(bbox.first.y),
                    //    static_cast<float>(bbox.first.z),
                    //    static_cast<float>(bbox.second.x),
                    //    static_cast<float>(bbox.second.y),
                    //    static_cast<float>(bbox.second.z),
                    //    }))
                    //    return;

                    ++draw_cnt;
                    command_buffers[thread_index].get().Draw(chunk.GetData(), chunk.GetWaterOffset());
                    if (!chunk.HasWater())
                        return;

                    std::lock_guard lock(water_mutex);
                    frustrum_passed_water_chunks.emplace_back(chunk);
                }, thread_index++ % thread_count));
            });

            for (auto& draw_thread : draw_threads)
            {
                draw_thread->Wait();
            }

            for (const auto& chunk : frustrum_passed_water_chunks)
            {
                draw_threads[thread_index++ % thread_count]->Add(std::bind([&](size_t thread_index) {
                    command_buffers[thread_index].get().Draw(
                        chunk.get().GetData(),
                        chunk.get().GetGpuSize() - chunk.get().GetWaterOffset(),
                        chunk.get().GetWaterOffset()
                    );
                }, thread_index++ % thread_count));
            }

            for (auto& draw_thread : draw_threads)
            {
                draw_thread->Wait();
            }
        }

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
