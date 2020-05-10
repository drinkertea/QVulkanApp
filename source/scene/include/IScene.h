#pragma once

#include <vector>
#include <string>
#include <memory>

namespace Vulkan
{

struct IFactory;
struct ICamera;

}

namespace Scene
{

struct IResourceLoader;

struct IScene
{
    virtual void Render() = 0;
    virtual const std::string& GetInfo() const = 0;

    virtual ~IScene() = default;

    static std::unique_ptr<IScene> Create(
        Vulkan::ICamera& camera,
        std::unique_ptr<Vulkan::IFactory> fac,
        std::unique_ptr<IResourceLoader> load
    );
};

}
