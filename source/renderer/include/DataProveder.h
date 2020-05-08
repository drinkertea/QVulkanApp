#pragma once

#include "IRenderer.h"

#include <vector>

namespace Vulkan
{

template <typename T>
class BufferDataOwner
    : public IDataProvider
{
    const std::vector<T>& data;

public:
    explicit BufferDataOwner(const std::vector<T>& inst)
        : data(inst)
    {
    }
    ~BufferDataOwner() override = default;

    uint32_t GetWidth() const override
    {
        return static_cast<uint32_t>(data.size());
    }

    const uint8_t* GetData() const override
    {
        return reinterpret_cast<const uint8_t*>(data.data());
    }

    uint32_t GetSize() const override
    {
        return GetWidth() * sizeof(T);
    }

    uint32_t GetHeight() const override { return 1u; }
    uint32_t GetDepth()  const override { return 1u; }
};

}
