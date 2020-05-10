#include <QFile>

#include <IFactory.h>
#include <ICamera.h>

#include <IScene.h>
#include <IResourceLoader.h>

#include "RenderInterface.h"

std::unique_ptr<Scene::IResourceLoader> CreateLoader();

class VulkanRenderer
    : public IVulkanRenderer
{
    QVulkanWindow& m_window;

    std::unique_ptr<Vulkan::ICamera> camera;
    std::unique_ptr<Scene::IScene> scene;
public:
    VulkanRenderer(QVulkanWindow& window)
        : m_window(window)
        , camera(Vulkan::CreateCamera())
    {
        float aspect = static_cast<float>(window.width()) / static_cast<float>(window.height());
        camera->SetPosition(0.0f, 80.0f, 0.0f);
        camera->SetRotation(-20.0f, 180.0f, 0.0f);
        camera->SetPerspective(60.0f, aspect, 0.1f, 4096.0f);
    }

    void initResources() override
    {
        scene = Scene::IScene::Create(*camera, CreateFactory(m_window), CreateLoader());
    }

    void releaseResources() override
    {
        scene = nullptr;
    }

    std::string GetWindowTitle()
    {
        std::string device(m_window.physicalDeviceProperties()->deviceName);
        std::string windowTitle;
        windowTitle = "QVulkanApp - " + device;
        windowTitle += scene->GetInfo();
        windowTitle += camera->GetInfo();
        return windowTitle;
    }

    void startNextFrame() override
    {
        scene->Render();
        m_window.setTitle(GetWindowTitle().c_str());
        m_window.frameReady();
        m_window.requestUpdate();
    }

    void OnKeyPressed(Qt::Key key) override
    {
        switch (key)
        {
        case Qt::Key::Key_W:     return camera->OnKeyPressed(Vulkan::Key::W);
        case Qt::Key::Key_S:     return camera->OnKeyPressed(Vulkan::Key::S);
        case Qt::Key::Key_A:     return camera->OnKeyPressed(Vulkan::Key::A);
        case Qt::Key::Key_D:     return camera->OnKeyPressed(Vulkan::Key::D);
        case Qt::Key::Key_Shift: return camera->OnKeyPressed(Vulkan::Key::Shift);
        }
    }

    void OnKeyReleased(Qt::Key key) override
    {
        switch (key)
        {
        case Qt::Key::Key_W:     return camera->OnKeyReleased(Vulkan::Key::W);
        case Qt::Key::Key_S:     return camera->OnKeyReleased(Vulkan::Key::S);
        case Qt::Key::Key_A:     return camera->OnKeyReleased(Vulkan::Key::A);
        case Qt::Key::Key_D:     return camera->OnKeyReleased(Vulkan::Key::D);
        case Qt::Key::Key_Shift: return camera->OnKeyReleased(Vulkan::Key::Shift);
        }
    }

    void OnMouseMove(int32_t x, int32_t y, const Qt::MouseButtons& buttons) override
    {
        uint32_t buttons_flags = 0u;

        if (buttons & Qt::MouseButton::LeftButton)
            buttons_flags |= Vulkan::MouseButtons::Left;

        if (buttons & Qt::MouseButton::RightButton)
            buttons_flags |= Vulkan::MouseButtons::Right;

        if (buttons & Qt::MouseButton::MiddleButton)
            buttons_flags |= Vulkan::MouseButtons::Middle;

        camera->OnMouseMove(x, y, static_cast<Vulkan::MouseButtons>(buttons_flags));
    }
};

std::unique_ptr<IVulkanRenderer> IVulkanRenderer::Create(QVulkanWindow& window)
{
    return std::make_unique<VulkanRenderer>(window);
}
