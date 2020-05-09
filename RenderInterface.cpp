#include <QFile>

#include <IRenderer.h>
#include <IFactory.h>
#include <ICamera.h>
#include <DataProveder.h>

#include <Noise.h>
#include <Chunk.h>
#include <Texture.h>

#include "RenderInterface.h"

std::vector<uint8_t> GetShaderData(const QString& url)
{
    QFile file(url);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Incalid path: " + url.toStdString());

    QByteArray blob = file.readAll();
    file.close();
    return { blob.begin(), blob.end() };
}

class VulkanRenderer
    : public IVulkanRenderer
{
    QVulkanWindow& m_window;

    std::unique_ptr<Vulkan::IFactory> factory;
    std::unique_ptr<Vulkan::ICamera> camera;
    Vulkan::IIndexBuffer*   index_buffer = nullptr;
    Vulkan::IVertexBuffer*  vertex_buffer = nullptr;
    Vulkan::IUniformBuffer* uniform_buffer = nullptr;
    Vulkan::IDescriptorSet* descriptor_set = nullptr;
    Vulkan::IPipeline*      pipeline = nullptr;

    std::vector<Scene::Chunk> chunks;

    std::unique_ptr<INoise> noiser;

    std::unique_ptr<Scene::Texture> textures;

    int draw_cnt = 0;
public:
    VulkanRenderer(QVulkanWindow& window)
        : m_window(window)
    {
        camera = Vulkan::CreateCamera();

        camera->SetPosition(0.0f, 80.0f, 0.0f);
        camera->SetRotation(-20.0f, 180.0f, 0.0f);
        camera->SetPerspective(60.0f, (float)window.width() / (float)window.height(), 0.1f, 1024.0f);
    }

    void initResources() override
    {
        factory = CreateFactory(m_window);

        Vulkan::Attributes attribs;
        attribs.push_back(Vulkan::AttributeFormat::vec1i);

        enum class Corner : uint32_t
        {
            TopLeft = 0,
            BotLeft,
            BotRight,
            TopRight,
        };

        std::vector<Corner> vertices = {
            Corner::TopLeft,
            Corner::BotLeft,
            Corner::BotRight,
            Corner::TopRight,
        };

        std::vector<uint32_t> indices = {
            0,1,2, 2,3,0,
        };

        vertex_buffer = &factory->CreateVertexBuffer(Vulkan::BufferDataOwner<Corner>(vertices), attribs);
        index_buffer  = &factory->CreateIndexBuffer(Vulkan::BufferDataOwner<uint32_t>(indices));

        noiser = INoise::CreateNoise(331132, 0.5f);
        int n = 4;
        for (int i = -n; i < n; ++i)
        {
            for (int j = -n; j < n; ++j)
            {
                chunks.emplace_back(Scene::Point2D{ i, j }, *factory, *noiser);
            }
        }

        using Scene::TextureType;
        auto texture_loader = CreateTextureLoader();
        textures = std::make_unique<Scene::Texture>(TextureType::First, Scene::texture_type_count, *texture_loader, *factory);

        Vulkan::InputResources bindings = {
            camera->GetMvpLayout(),
            textures->GetTexture(),
        };

        descriptor_set = &factory->CreateDescriptorSet(bindings);

        auto& vertex = factory->CreateShader(Vulkan::BufferDataOwner<uint8_t>(GetShaderData(":/texture.vert.spv")), Vulkan::ShaderType::vertex);
        auto& fragment = factory->CreateShader(Vulkan::BufferDataOwner<uint8_t>(GetShaderData(":/texture.frag.spv")), Vulkan::ShaderType::fragment);
        Vulkan::Shaders shaders = { vertex, fragment };

        pipeline = &factory->CreatePipeline(*descriptor_set, shaders, *vertex_buffer, chunks.front().GetData());
    }

    void releaseResources() override
    {
        factory = nullptr;
        index_buffer = nullptr;
        vertex_buffer = nullptr;
        uniform_buffer = nullptr;
        descriptor_set = nullptr;
        pipeline = nullptr;
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

    void Render()
    {
        auto render_pass = factory->CreateRenderPass(*camera);
        render_pass->Bind(*descriptor_set);
        render_pass->Bind(*pipeline);
        render_pass->Bind(*index_buffer, *vertex_buffer);

        draw_cnt = 0;
        for (const auto& chunk : chunks)
        {
            const auto& bbox = chunk.GetBBox();
            if (!camera->ObjectVisible(Vulkan::BBox{
                static_cast<float>(bbox.first.x),
                static_cast<float>(bbox.first.y),
                static_cast<float>(bbox.first.z),
                static_cast<float>(bbox.second.x),
                static_cast<float>(bbox.second.y),
                static_cast<float>(bbox.second.z),
            }))
                continue;

            ++draw_cnt;
            render_pass->Draw(chunk.GetData());
        }
    }

    std::string GetWindowTitle()
    {
        std::string device(m_window.physicalDeviceProperties()->deviceName);
        std::string windowTitle;
        windowTitle = "QVulkanApp - " + device;
        windowTitle += " - " + std::to_string(draw_cnt) + " chunks ";
        windowTitle += camera->GetInfo();
        return windowTitle;
    }

    void startNextFrame() override
    {
        Render();
        m_window.setTitle(GetWindowTitle().c_str());
        m_window.frameReady();
        m_window.requestUpdate();
    }
};

std::unique_ptr<IVulkanRenderer> IVulkanRenderer::Create(QVulkanWindow& window)
{
    return std::make_unique<VulkanRenderer>(window);
}
