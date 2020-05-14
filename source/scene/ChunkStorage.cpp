#include "ChunkStorage.h"

#include <IFactory.h>
#include <ICamera.h>
#include <Noise.h>

#include "Chunk.h"
#include "ThreadUtils.hpp"

namespace Scene
{

static const utils::vec2i g_invalid_pos = { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min() };

struct ChunkWrapper
{
    utils::vec2i mid{};
    utils::vec2i pos{};
    ChunkPtr     chunk;
};

class ChunkStorage
    : public IChunkStorage
{
    Vulkan::ICamera& camera;
    Vulkan::IFactory& factory;
    utils::DefferedExecutor gpu_creation_pool;

    std::unique_ptr<INoise> noiser;

    static constexpr int32_t render_distance = 16;
    static constexpr int32_t squere_len = render_distance * 2 + 1;

    using Chunks = std::vector<std::vector<ChunkPtr>>;
    using FutureChunks = std::vector<std::future<ChunkWrapper>>;
    Chunks chunks;
    FutureChunks future_chunks;

    utils::vec2i current_chunk = g_invalid_pos;

    utils::PriorityExecutor<ChunkWrapper> cpu_creation_pool;

    uint64_t frame_number = 0;

    ChunkPtr& GetChunk(const utils::vec2i& mid, const utils::vec2i& pos)
    {
        return chunks[render_distance + pos.x - mid.x][render_distance + pos.y - mid.y];
    }

    const ChunkPtr& GetChunk(const utils::vec2i& mid, const utils::vec2i& pos) const
    {
        return chunks[render_distance + pos.x - mid.x][render_distance + pos.y - mid.y];
    }

public:
    utils::vec2i GetCamPos() const
    {
        auto pos = camera.GetViewPos();
        return WorldToChunk({ static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.z) });
    }

    ChunkStorage(Vulkan::ICamera& camera, Vulkan::IFactory& factory)
        : camera(camera)
        , factory(factory)
        , noiser(INoise::CreateNoise(213312, 0.5f))
    {
        chunks.resize(squere_len);
        for (auto& x : chunks)
            x.resize(squere_len);

        future_chunks.resize(squere_len * squere_len);

        DoCpuWork();
        const auto& chunk = GetChunk(current_chunk, current_chunk);
        while (!chunk)
        {
            UpdateChunks();
            std::this_thread::sleep_for(std::chrono::microseconds(1u));
        }
        DoGpuWork();
    }

    const Vulkan::IInstanceBuffer& GetInstanceLayout() const override
    {
        const auto& chunk = GetChunk(current_chunk, current_chunk);
        if (!chunk || !chunk->Ready())
            throw std::logic_error("First chunk must be loaded!");
        return chunk->GetData();
    }

    void DoCpuWork()
    {
        auto cam_chunk = GetCamPos();
        if (cam_chunk == current_chunk)
            return UpdateChunks();

        utils::vec2i translation = cam_chunk - current_chunk;

        if (current_chunk != g_invalid_pos && translation != utils::vec2i(0, 0))
        {
            utils::ShiftPlane(translation, chunks);
        }

        current_chunk = cam_chunk;
        utils::IterateFromMid(render_distance, current_chunk, [&](int index, const utils::vec2i& pos) {
            const auto& chunk = GetChunk(current_chunk, pos);
            if (chunk)
                return;

            future_chunks[index] = cpu_creation_pool.Add(index,
                std::bind([this](const utils::vec2i& mid, const utils::vec2i& pos) -> ChunkWrapper {
                    if (mid != current_chunk)
                        return { g_invalid_pos, g_invalid_pos, nullptr };
                    return { mid, pos, std::make_unique<Chunk>(pos, factory, *noiser, gpu_creation_pool) };
                },
                current_chunk,
                pos
            ));
        });

        UpdateChunks();
    }

    void UpdateChunks()
    {
        for (auto& future_chunk : future_chunks)
        {
            bool ready = future_chunk.valid() && future_chunk.wait_for(std::chrono::milliseconds(0u)) == std::future_status::ready;
            if (!ready)
                continue;

            auto data = future_chunk.get();
            if (!data.chunk || current_chunk != data.mid)
                continue;

            GetChunk(current_chunk, data.pos).swap(data.chunk);
        }
    }

    void DoGpuWork()
    {
        gpu_creation_pool.Execute(frame_number++);
    }

    void OnRender() override
    {
        DoCpuWork();
        DoGpuWork();
    }

    void ForEach(const std::function<void(const Chunk&)>& callback) override
    {
        utils::IterateFromMid(render_distance, current_chunk, [&](int index, const utils::vec2i& pos) {
            const auto& chunk = GetChunk(current_chunk, pos);
            if (!chunk || !chunk->Ready())
                return;

            callback(*chunk);
        });
    }
};

std::unique_ptr<IChunkStorage> IChunkStorage::Create(Vulkan::ICamera& camera, Vulkan::IFactory& factory)
{
    return std::make_unique<ChunkStorage>(camera, factory);
}

}
