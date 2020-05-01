
#include <QGuiApplication>
#include <QVulkanInstance>
#include <QLoggingCategory>
#include <QVulkanWindow>
#include <QMouseEvent>

#include "RenderInterface.h"

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

class VulkanWindow : public QVulkanWindow
{
    IVulkanRenderer* renderer = nullptr;
public:
    QVulkanWindowRenderer* createRenderer() override
    {
        renderer = IVulkanRenderer::Create(*this).release();
        return renderer;
    }

    void mouseMoveEvent(QMouseEvent* evnt) override
    {
        if (!renderer || !evnt)
            return;

        renderer->OnMouseMove(evnt->x(), evnt->y(), evnt->buttons());
    }

    void keyPressEvent(QKeyEvent* evnt) override
    {
        if (!renderer || !evnt)
            return;

        renderer->OnKeyPressed(static_cast<Qt::Key>(evnt->key()));
    }

    void keyReleaseEvent(QKeyEvent* evnt) override
    {
        if (!renderer || !evnt)
            return;

        renderer->OnKeyReleased(static_cast<Qt::Key>(evnt->key()));
    }

};

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    QVulkanInstance inst;

#ifndef Q_OS_ANDROID
    inst.setLayers(QByteArrayList()
        << "VK_LAYER_KHRONOS_validation"
    );
#else
    inst.setLayers(QByteArrayList()
        << "VK_LAYER_GOOGLE_threading"
        << "VK_LAYER_LUNARG_parameter_validation"
        << "VK_LAYER_LUNARG_object_tracker"
        << "VK_LAYER_LUNARG_core_validation"
        << "VK_LAYER_LUNARG_image"
        << "VK_LAYER_LUNARG_swapchain"
        << "VK_LAYER_GOOGLE_unique_objects"
    );
#endif

    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    VulkanWindow w;
    w.setVulkanInstance(&inst);

    w.resize(1280, 720);
    w.show();

    return app.exec();
}
