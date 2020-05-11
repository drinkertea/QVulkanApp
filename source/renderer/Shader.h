#pragma once

#include <vulkan/vulkan.h>
#include "IRenderer.h"
#include "IFactory.h"

class QVulkanDeviceFunctions;
class QVulkanWindow;

namespace Vulkan
{

class Shader
    : public IShader
{
public:
    Shader(const IDataProvider& data, ShaderType type, const QVulkanWindow& window);
    ~Shader() override;

    VkShaderModule GetModule() const;
    VkShaderStageFlagBits GetStage() const;

private:
    QVulkanDeviceFunctions& functions;
    VkDevice device = nullptr;

    VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;
    VkShaderModule shader_module = nullptr;
};

}