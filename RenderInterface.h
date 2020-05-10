#pragma once

#include <QVulkanWindow>

struct IVulkanRenderer
    : public QVulkanWindowRenderer
{
    static std::unique_ptr<IVulkanRenderer> Create(QVulkanWindow& window);

    virtual void OnMouseMove(int32_t x, int32_t y, const Qt::MouseButtons& buttons) = 0;
    virtual void OnKeyPressed(Qt::Key key) = 0;
    virtual void OnKeyReleased(Qt::Key key) = 0;
    
    ~IVulkanRenderer() override = default;
};

namespace Scene
{
struct ITextureLoader;
struct IShaderLoader;
}

std::unique_ptr<Scene::ITextureLoader> CreateTextureLoader();
std::unique_ptr<Scene::IShaderLoader> CreateShaderLoader();
