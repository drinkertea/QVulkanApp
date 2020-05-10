#include "DataProveder.h"
#include "Shader.h"

#include "include/IResourceLoader.h"

namespace Scene
{

Program::Program(ShaderTarget target, const IResourceLoader& loader, Vulkan::IFactory& factory)
{
    switch (target)
    {
    case ShaderTarget::Block:
        shaders.push_back(
            factory.CreateShader(
                Vulkan::BufferDataOwner<uint8_t>(loader.LoadShader(target, Vulkan::ShaderType::vertex)),
                Vulkan::ShaderType::vertex
            )
        );
        shaders.push_back(
            factory.CreateShader(
                Vulkan::BufferDataOwner<uint8_t>(loader.LoadShader(target, Vulkan::ShaderType::fragment)),
                Vulkan::ShaderType::fragment
            )
        );
        break;
    }
}

const Vulkan::Shaders& Program::GetShaders() const
{
    return shaders;
}

}
