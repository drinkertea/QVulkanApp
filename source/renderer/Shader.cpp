#include "Utils.h"

#include <QVulkanFunctions>
#include <QVulkanWindow>
#include "Shader.h"

namespace Vulkan
{
    VkShaderStageFlagBits GetVulkanStage(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderType::fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
        default: throw std::logic_error("Invalid enum value");
        }
    }

    Shader::Shader(const IDataProvider& data, ShaderType type, const QVulkanWindow& window)
        : device(window.device())
        , functions(*window.vulkanInstance()->deviceFunctions(window.device()))
        , stage(GetVulkanStage(type))
    {
        VkShaderModuleCreateInfo shader_info{};
        shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_info.codeSize = data.GetSize();
        shader_info.pCode = reinterpret_cast<const uint32_t*>(data.GetData());

        Vulkan::VkResultSuccess(functions.vkCreateShaderModule(device, &shader_info, nullptr, &shader_module));
    }

    Shader::~Shader()
    {
        functions.vkDestroyShaderModule(device, shader_module, nullptr);
    }

    VkShaderModule Shader::GetModule() const
    {
        return shader_module;
    }

    VkShaderStageFlagBits Shader::GetStage() const
    {
        return stage;
    }
}
