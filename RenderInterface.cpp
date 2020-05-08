#include <QFile>
#include <QVector2D>
#include <QVulkanFunctions>

#include <IRenderer.h>
#include <IFactory.h>
#include <DataProveder.h>

#include <Noise.h>
#include <Chunk.h>
#include <Texture.h>

#include <iostream>
#include <algorithm>

#include "RenderInterface.h"
#include "Camera.h"

std::vector<uint8_t> GetCameraData(Camera& camera)
{
    constexpr size_t mat_size = 16 * sizeof(float);
    constexpr size_t vec_size = 4 * sizeof(float);

    std::vector<uint8_t> res(mat_size * 2 + vec_size);
    memcpy(&res[0], camera.matrices.perspective.constData(), mat_size);
    memcpy(&res[mat_size], camera.matrices.view.constData(), mat_size);
    memcpy(&res[mat_size * 2], &camera.viewPos[0], vec_size);
    return res;
}

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
    Vulkan::IIndexBuffer*   index_buffer = nullptr;
    Vulkan::IVertexBuffer*  vertex_buffer = nullptr;
    Vulkan::IUniformBuffer* uniform_buffer = nullptr;
    Vulkan::IDescriptorSet* descriptor_set = nullptr;
    Vulkan::IPipeline*      pipeline = nullptr;

    std::vector<Scene::Chunk> chunks;

    std::unique_ptr<INoise> noiser;

    std::unique_ptr<Scene::Texture> textures;

    Camera camera;

    int draw_cnt = 0;
    QVector2D mouse_pos;
    bool view_updated = false;
    uint32_t frame_counter = 0;
    float frame_timer = 1.0f;
    float timer = 0.0f;
    float timer_speed = 0.25f;
    bool paused = false;
    uint32_t lastFPS = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp;

public:
    VulkanRenderer(QVulkanWindow& window)
        : m_window(window)
    {
        camera.type = Camera::CameraType::firstperson;
        camera.setPosition(QVector3D(0.0f, 80.0f, 0.0f));
        camera.setRotation(QVector3D(-20.0f, 180.0f, 0.0f));
        camera.setPerspective(60.0f, (float)window.width() / (float)window.height(), 0.1f, 1024.0f);
        camera.movementSpeed = 10;
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
        int n = 16;
        for (int i = -n; i < n; ++i)
        {
            for (int j = -n; j < n; ++j)
            {
                chunks.emplace_back(Scene::Point2D{ i, j }, *factory, *noiser);
            }
        }

        uniform_buffer = &factory->CreateUniformBuffer(Vulkan::BufferDataOwner<uint8_t>(GetCameraData(camera)));

        using Scene::TextureType;
        auto texture_loader = CreateTextureLoader();
        textures = std::make_unique<Scene::Texture>(TextureType::First, Scene::texture_type_count, *texture_loader, *factory);

        Vulkan::InputResources bindings = {
            *uniform_buffer,
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
        if (!camera.firstperson)
            return;

        switch (key)
        {
        case Qt::Key::Key_W:
            camera.keys.up = true;
            break;
        case Qt::Key::Key_S:
            camera.keys.down = true;
            break;
        case Qt::Key::Key_A:
            camera.keys.left = true;
            break;
        case Qt::Key::Key_D:
            camera.keys.right = true;
            break;
        case Qt::Key::Key_Shift:
            camera.movementSpeed = 100;
            break;
        }
    }

    void OnKeyReleased(Qt::Key key) override
    {
        if (!camera.firstperson)
            return;

        switch (key)
        {
        case Qt::Key::Key_W:
            camera.keys.up = false;
            break;
        case Qt::Key::Key_S:
            camera.keys.down = false;
            break;
        case Qt::Key::Key_A:
            camera.keys.left = false;
            break;
        case Qt::Key::Key_D:
            camera.keys.right = false;
            break;
        case Qt::Key::Key_Shift:
            camera.movementSpeed = 10;
            break;
        }
    }

    void OnMouseMove(int32_t x, int32_t y, const Qt::MouseButtons& buttons) override
    {
        int32_t dx = static_cast<int32_t>(mouse_pos.x()) - x;
        int32_t dy = static_cast<int32_t>(mouse_pos.y()) - y;

        if (buttons & Qt::MouseButton::LeftButton)
        {
            camera.rotate(QVector3D(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
            view_updated = true;
        }
        if (buttons & Qt::MouseButton::RightButton)
        {
            camera.translate(QVector3D(-0.0f, 0.0f, dy * .005f));
            view_updated = true;
        }
        if (buttons & Qt::MouseButton::MiddleButton)
        {
            camera.translate(QVector3D(-dx * 0.01f, -dy * 0.01f, 0.0f));
            view_updated = true;
        }
        mouse_pos.setX(x);
        mouse_pos.setY(y);
    }

    void Render()
    {
        {
            auto render_pass = factory->CreateRenderPass();
            render_pass->Bind(*descriptor_set);
            render_pass->Bind(*pipeline);
            render_pass->Bind(*index_buffer, *vertex_buffer);

            Frustum frustum;
            frustum.update(camera.matrices.perspective * camera.matrices.view);

            draw_cnt = 0;
            for (const auto& chunk : chunks)
            {
                const auto& bbox = chunk.GetBBox();
                if (!frustum.checkBox(AAbox({
                    static_cast<float>(bbox.first.x),
                    static_cast<float>(bbox.first.y),
                    static_cast<float>(bbox.first.z)
                }, {
                    static_cast<float>(bbox.second.x),
                    static_cast<float>(bbox.second.y),
                    static_cast<float>(bbox.second.z)
                })))
                    continue;

                ++draw_cnt;
                render_pass->Draw(chunk.GetData());
            }
        }
        m_window.frameReady();
        m_window.requestUpdate();
    }

    std::string GetWindowTitle()
    {
        std::string device(m_window.physicalDeviceProperties()->deviceName);
        std::string windowTitle;
        windowTitle = "QVulkanApp - " + device;
        windowTitle += " - " + std::to_string(frame_counter) + " fps";
        windowTitle += " - " + std::to_string(draw_cnt) + " chunks ";
        windowTitle += " - " + std::to_string(camera.viewPos.x()) + "; "
            + std::to_string(camera.viewPos.y()) + "; "
            + std::to_string(camera.viewPos.z()) + "; ";

        windowTitle += " - " + std::to_string(camera.rotation.x()) + "; "
            + std::to_string(camera.rotation.y()) + "; "
            + std::to_string(camera.rotation.z()) + "; ";

        return windowTitle;
    }

    void startNextFrame() override
    {
        auto render_start = std::chrono::high_resolution_clock::now();
        if (view_updated)
        {
            view_updated = false;
            uniform_buffer->Update(Vulkan::BufferDataOwner<uint8_t>(GetCameraData(camera)));
        }

        Render();
        frame_counter++;
        auto render_end = std::chrono::high_resolution_clock::now();
        auto time_diff = std::chrono::duration<double, std::milli>(render_end - render_start).count();
        frame_timer = static_cast<float>(time_diff) / 1000.0f;
        camera.update(frame_timer);
        if (camera.moving())
        {
            view_updated = true;
        }

        if (!paused)
        {
            timer += timer_speed * frame_timer;
            if (timer > 1.0)
            {
                timer -= 1.0f;
            }
        }
        float fps_timer = static_cast<float>(std::chrono::duration<double, std::milli>(render_end - lastTimestamp).count());
        if (fps_timer > 1000.0f)
        {
            lastFPS = static_cast<uint32_t>(static_cast<float>(frame_counter) * (1000.0f / fps_timer));

            m_window.setTitle(GetWindowTitle().c_str());

            frame_counter = 0;
            lastTimestamp = render_end;
        }
    }
};

std::unique_ptr<IVulkanRenderer> IVulkanRenderer::Create(QVulkanWindow& window)
{
    return std::make_unique<VulkanRenderer>(window);
}
