#include <QFile>
#include <QVector2D>
#include <QVulkanFunctions>

#include "RenderInterface.h"
#include "Camera.h"

#include "IRenderer.h"
#include "IFactory.h"

struct IvalidPath : public std::runtime_error
{
    IvalidPath(const std::string& msg)
        : std::runtime_error("Cannot find the path specified: " + msg)
    {
    }
};

class TextureSource
    : public Vulkan::IDataProvider
{
    std::vector<uint8_t> data;
    uint32_t width = 0;
    uint32_t height = 0;

public:
    TextureSource(const QString& url)
    {
        QImage image(url);
        if (image.isNull())
            throw IvalidPath(url.toStdString());

        image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

        width = image.width();
        height = image.height();
        data.resize(height * width * 4);
        auto mapped_data = data.data();
        for (uint32_t y = 0; y < height; ++y)
        {
            auto row_pitch = width * 4; // todo: provide format or row pith
            memcpy(mapped_data, image.constScanLine(y), row_pitch);
            mapped_data += row_pitch;
        }
    }

    ~TextureSource() override = default;

    uint32_t GetWidth() const override
    {
        return width;
    }

    uint32_t GetHeight() const override
    {
        return height;
    }

    const uint8_t* GetData() const override
    {
        return data.data();
    }

    uint32_t GetSize() const override
    {
        return static_cast<uint32_t>(data.size());
    }

    uint32_t GetDepth() const override
    {
        return 1;
    }
};

class VertexData
    : public Vulkan::IDataProvider
{
    struct Vertex {
        float pos[3];
        float uv[2];
        float normal[3];
    };

    std::vector<Vertex> vertices =
    {
        // front
        { { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        //back
        { { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        //left
        { { 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        //right
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        //bot
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        //top
        { { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },

    };

public:
    ~VertexData() override = default;

    uint32_t GetWidth() const override
    {
        return static_cast<uint32_t>(vertices.size());
    }

    uint32_t GetHeight() const override
    {
        return 1;
    }

    const uint8_t* GetData() const override
    {
        return reinterpret_cast<const uint8_t*>(vertices.data());
    }

    uint32_t GetSize() const override
    {
        return GetWidth() * sizeof(Vertex);
    }

    uint32_t GetDepth() const override
    {
        return 1;
    }
};

class IndexData
    : public Vulkan::IDataProvider
{
    std::vector<uint32_t> indices = {
        0,1,2, 2,3,0, // front
        0 + 4,1 + 4,2 + 4, 2 + 4,3 + 4,0 + 4, // back
        0 + 8,1 + 8,2 + 8, 2 + 8,3 + 8,0 + 8, // left
        0 + 12,1 + 12,2 + 12, 2 + 12,3 + 12,0 + 12, // back
        0 + 16,1 + 16,2 + 16, 2 + 16,3 + 16,0 + 16, // bot
        0 + 20,1 + 20,2 + 20, 2 + 20,3 + 20,0 + 20, // top
    };

public:
    ~IndexData() override = default;

    uint32_t GetWidth() const override
    {
        return static_cast<uint32_t>(indices.size());
    }

    uint32_t GetHeight() const override
    {
        return 1;
    }

    const uint8_t* GetData() const override
    {
        return reinterpret_cast<const uint8_t*>(indices.data());
    }

    uint32_t GetSize() const override
    {
        return GetWidth() * sizeof(uint32_t);
    }

    uint32_t GetDepth() const override
    {
        return 1;
    }
};

class UniformData
    : public Vulkan::IDataProvider
{
    struct {
        QMatrix4x4 projection;
        QMatrix4x4 model_view;
        QVector4D  view_pos;
    } m_ubo;

    std::vector<uint8_t> data;

public:
    ~UniformData() override = default;

    uint32_t GetWidth() const override
    {
        return 1;
    }

    uint32_t GetHeight() const override
    {
        return 1;
    }

    const uint8_t* GetData() const override
    {
        return reinterpret_cast<const uint8_t*>(data.data());
    }

    uint32_t GetSize() const override
    {
        return static_cast<uint32_t>(data.size());
    }

    uint32_t GetDepth() const override
    {
        return 1;
    }

    void Update(const Camera& camera)
    {
        constexpr size_t mat_size = 16 * sizeof(float);
        constexpr size_t vec_size = 4 * sizeof(float);

        m_ubo.projection = camera.matrices.perspective;
        m_ubo.model_view = camera.matrices.view;
        m_ubo.view_pos = camera.viewPos;

        data.resize(mat_size * 2 + vec_size);
        auto mapped = data.data();
        float t[4] = { m_ubo.view_pos[0], m_ubo.view_pos[1], m_ubo.view_pos[2], m_ubo.view_pos[3] };
        memcpy(mapped, m_ubo.projection.constData(), mat_size);
        memcpy(mapped + mat_size, m_ubo.model_view.constData(), mat_size);
        memcpy(mapped + 2 * mat_size, t, sizeof(t));
    }
};

class ShaderData
    : public Vulkan::IDataProvider
{
    QByteArray blob;

public:
    ShaderData(const QString& url)
    {
        QFile file(url);
        if (!file.open(QIODevice::ReadOnly))
            throw IvalidPath(url.toStdString());

        blob = file.readAll();
        file.close();
    }
    ~ShaderData() override = default;

    uint32_t GetWidth() const override
    {
        return 1;
    }

    uint32_t GetHeight() const override
    {
        return 1;
    }

    const uint8_t* GetData() const override
    {
        return reinterpret_cast<const uint8_t*>(blob.constData());
    }

    uint32_t GetSize() const override
    {
        return blob.size();
    }

    uint32_t GetDepth() const override
    {
        return 1;
    }
};

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

    UniformData uniform_data{};

    Camera camera;
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
        camera.setPosition(QVector3D(0.0f, 0.0f, -2.5f));
        camera.setRotation(QVector3D(0.0f, 15.0f, 0.0f));
        camera.setPerspective(60.0f, (float)window.width() / (float)window.height(), 0.1f, 256.0f);
    }

    void initResources() override
    {
        factory = CreateFactory(m_window);

        auto& dirt_texture = factory->CreateTexture(TextureSource(":/dirt.png"));

        Vulkan::Attributes attribs;
        attribs.push_back(Vulkan::AttributeFormat::vec3f);
        attribs.push_back(Vulkan::AttributeFormat::vec2f);
        attribs.push_back(Vulkan::AttributeFormat::vec3f);

        uniform_data.Update(camera);

        vertex_buffer = &factory->CreateVertexBuffer(VertexData{}, attribs);
        index_buffer  = &factory->CreateIndexBuffer(IndexData{});

        uniform_buffer = &factory->CreateUniformBuffer(uniform_data);

        Vulkan::InputResources bindings = {
            *uniform_buffer,
            dirt_texture,
        };

        descriptor_set = &factory->CreateDescriptorSet(bindings);

        auto& vertex = factory->CreateShader(ShaderData(":/texture.vert.spv"), Vulkan::ShaderType::vertex);
        auto& fragment = factory->CreateShader(ShaderData(":/texture.frag.spv"), Vulkan::ShaderType::fragment);
        Vulkan::Shaders shaders = { vertex, fragment };

        pipeline = &factory->CreatePipeline(*descriptor_set, shaders, *vertex_buffer);
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
            render_pass->Draw(*index_buffer, *vertex_buffer);
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
        return windowTitle;
    }

    void startNextFrame() override
    {
        auto render_start = std::chrono::high_resolution_clock::now();
        if (view_updated)
        {
            view_updated = false;
            uniform_data.Update(camera);
            uniform_buffer->Update(uniform_data);
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
