#include <QMatrix4x4>
#include <QVector2D>
#include <qmath.h>
#include <array>

#include "Camera.h"

class AAbox
{
    const Vulkan::BBox& bbox;
public:
    AAbox(const Vulkan::BBox& bbox)
        : bbox(bbox)
    {
    }

    QVector3D GetPositive(const QVector4D& normal) const
    {
        QVector3D p(bbox.min_x, bbox.min_y, bbox.min_z);
        if (normal.x() >= 0)
            p.setX(bbox.max_x);
        if (normal.y() >= 0)
            p.setY(bbox.max_y);
        if (normal.z() >= 0)
            p.setZ(bbox.max_z);
        return p;
    }

    QVector3D GetNegative(const QVector4D& normal) const
    {
        QVector3D p(bbox.max_x, bbox.max_y, bbox.max_z);
        if (normal.x() >= 0)
            p.setX(bbox.min_x);
        if (normal.y() >= 0)
            p.setY(bbox.min_y);
        if (normal.z() >= 0)
            p.setZ(bbox.min_z);
        return p;
    }
};

class Frustum
{
public:
    enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };
    std::array<QVector4D, 6> planes;

    void update(const QMatrix4x4& model)
    {
        std::array<QVector4D, 4> matrix = {
            model.column(0),
            model.column(1),
            model.column(2),
            model.column(3),
        };
        planes[LEFT].setX(matrix[0].w() + matrix[0].x());
        planes[LEFT].setY(matrix[1].w() + matrix[1].x());
        planes[LEFT].setZ(matrix[2].w() + matrix[2].x());
        planes[LEFT].setW(matrix[3].w() + matrix[3].x());

        planes[RIGHT].setX(matrix[0].w() - matrix[0].x());
        planes[RIGHT].setY(matrix[1].w() - matrix[1].x());
        planes[RIGHT].setZ(matrix[2].w() - matrix[2].x());
        planes[RIGHT].setW(matrix[3].w() - matrix[3].x());

        planes[TOP].setX(matrix[0].w() - matrix[0].y());
        planes[TOP].setY(matrix[1].w() - matrix[1].y());
        planes[TOP].setZ(matrix[2].w() - matrix[2].y());
        planes[TOP].setW(matrix[3].w() - matrix[3].y());

        planes[BOTTOM].setX(matrix[0].w() + matrix[0].y());
        planes[BOTTOM].setY(matrix[1].w() + matrix[1].y());
        planes[BOTTOM].setZ(matrix[2].w() + matrix[2].y());
        planes[BOTTOM].setW(matrix[3].w() + matrix[3].y());

        planes[BACK].setX(matrix[0].w() + matrix[0].z());
        planes[BACK].setY(matrix[1].w() + matrix[1].z());
        planes[BACK].setZ(matrix[2].w() + matrix[2].z());
        planes[BACK].setW(matrix[3].w() + matrix[3].z());

        planes[FRONT].setX(matrix[0].w() - matrix[0].z());
        planes[FRONT].setY(matrix[1].w() - matrix[1].z());
        planes[FRONT].setZ(matrix[2].w() - matrix[2].z());
        planes[FRONT].setW(matrix[3].w() - matrix[3].z());

        for (auto i = 0; i < planes.size(); i++)
        {
            planes[i].normalize();
        }
    }

    static float distance(const QVector4D& plane, const QVector3D& pos)
    {
        return plane.x() * pos.x() + plane.y() * pos.y() + plane.z() * pos.z() + plane.w();
    }

    bool checkBox(const AAbox& box) const
    {
        for (auto i = 0; i < planes.size(); i++)
        {
            if (distance(planes[i], box.GetPositive(planes[i])) <= 0)
                return false;

            // Intersection check
            // else if (distance(planes[i], box.GetNegative(planes[i])) <= 0)
            //     return true;
        }
        return true;
    }
};

class Camera
{
private:
    float fov;
    float znear, zfar;

    void updateViewMatrix()
    {
        QMatrix4x4 rotM;
        QMatrix4x4 transM;

        rotM.rotate(rotation[0] * (flipY ? -1.0f : 1.0f), QVector3D(1.0f, 0.0f, 0.0f));
        rotM.rotate(rotation[1], QVector3D(0.0f, 1.0f, 0.0f));
        rotM.rotate(rotation[2], QVector3D(0.0f, 0.0f, 1.0f));

        QVector3D translation = position;
        if (flipY)
        {
            translation[1] *= -1.0f;
        }
        transM.translate(translation);

        if (type == CameraType::firstperson)
        {
            matrices.view = rotM * transM;
        }
        else
        {
            matrices.view = transM * rotM;
        }

        viewPos = QVector4D(position, 0.0f) * QVector4D(-1.0f, 1.0f, -1.0f, 1.0f);

        updated = true;
    };
public:
    enum CameraType { lookat, firstperson };
    CameraType type = CameraType::lookat;

    QVector3D rotation;
    QVector3D position;
    QVector4D viewPos;

    float rotationSpeed = 1.0f;
    float movementSpeed = 1.0f;

    bool updated = false;
    bool flipY = true;

    struct
    {
        QMatrix4x4 perspective;
        QMatrix4x4 view;
    } matrices;

    struct
    {
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
    } keys;

    bool moving()
    {
        return keys.left || keys.right || keys.up || keys.down;
    }

    float getNearClip() {
        return znear;
    }

    float getFarClip() {
        return zfar;
    }

    void setPerspective(float fov, float aspect, float znear, float zfar)
    {
        this->fov = fov;
        this->znear = znear;
        this->zfar = zfar;
        matrices.perspective.perspective(fov, aspect, znear, zfar);
        if (flipY) {
            auto t = matrices.perspective.row(1);
            t[1] *= -1.0f;
            matrices.perspective.setRow(1, t);
        }
    };

    void updateAspectRatio(float aspect)
    {
        matrices.perspective.perspective(fov, aspect, znear, zfar);
        if (flipY) {
            auto t = matrices.perspective.row(1);
            t[1] *= -1.0f;
            matrices.perspective.setRow(1, t);
        }
    }

    void setPosition(QVector3D position)
    {
        this->position = position;
        updateViewMatrix();
    }

    void setRotation(QVector3D rotation)
    {
        this->rotation = rotation;
        updateViewMatrix();
    }

    void rotate(QVector3D delta)
    {
        this->rotation += delta;
        updateViewMatrix();
    }

    void setTranslation(QVector3D translation)
    {
        this->position = translation;
        updateViewMatrix();
    };

    void translate(QVector3D delta)
    {
        this->position += delta;
        updateViewMatrix();
    }

    void setRotationSpeed(float rotationSpeed)
    {
        this->rotationSpeed = rotationSpeed;
    }

    void setMovementSpeed(float movementSpeed)
    {
        this->movementSpeed = movementSpeed;
    }

    void update(float deltaTime)
    {
        updated = false;
        if (type == CameraType::firstperson)
        {
            if (moving())
            {
                QVector3D camFront;
                camFront.setX(-qCos(qDegreesToRadians(rotation[0])) * qSin(qDegreesToRadians(rotation[1])));
                camFront.setY(qSin(qDegreesToRadians(rotation[0])));
                camFront.setZ(qCos(qDegreesToRadians(rotation[0])) * qCos(qDegreesToRadians(rotation[1])));
                camFront.normalize();

                float moveSpeed = deltaTime * movementSpeed;

                if (keys.up)
                    position += camFront * moveSpeed;
                if (keys.down)
                    position -= camFront * moveSpeed;
                if (keys.left)
                    position -= QVector3D::crossProduct(camFront, QVector3D(0.0f, 1.0f, 0.0f)).normalized() * moveSpeed;
                if (keys.right)
                    position += QVector3D::crossProduct(camFront, QVector3D(0.0f, 1.0f, 0.0f)).normalized() * moveSpeed;

                updateViewMatrix();
            }
        }
    };
};

namespace Vulkan
{
    class Camera
        : public CameraRaii
    {
        ::Camera camera;
        ::Frustum frustum;

        QVector2D mouse_pos;

        QMatrix4x4 mvp;
        bool view_updated = true;
        std::chrono::time_point<std::chrono::steady_clock> start_tp;
        uint32_t frame_counter = 0;
        float frame_timer = 1.0f;
        float timer = 0.0f;
        float timer_speed = 0.25f;
        bool paused = false;
        uint32_t lastFPS = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_timestamp;
        std::string info;

        PushConstantLayout mvp_layout{ 0u, sizeof(float) * 16u };

        static constexpr float speed = 100.f;

    public:
        Camera()
        {
            camera.type = ::Camera::CameraType::firstperson;
            camera.movementSpeed = speed;
        }

        void SetPosition(float x, float y, float z) override
        {
            camera.setPosition(QVector3D(x, y, z));
        }

        void SetRotation(float x, float y, float z) override
        {
            camera.setRotation(QVector3D(x, y, z));
        }

        void SetPerspective(float fov, float aspect, float znear, float zfar) override
        {
            camera.setPerspective(fov, aspect, znear, zfar);
        }

        void UpdateAspectRatio(float aspect) override
        {
            camera.updateAspectRatio(aspect);
        }

        void OnKeyPressed(Key key) override
        {
            if (!camera.firstperson)
                return;

            switch (key)
            {
            case Key::W:
                camera.keys.up = true;
                break;
            case Key::S:
                camera.keys.down = true;
                break;
            case Key::A:
                camera.keys.left = true;
                break;
            case Key::D:
                camera.keys.right = true;
                break;
            case Key::Shift:
                camera.movementSpeed = speed * 10;
                break;
            }
        }

        void OnKeyReleased(Key key) override
        {
            if (!camera.firstperson)
                return;

            switch (key)
            {
            case Key::W:
                camera.keys.up = false;
                break;
            case Key::S:
                camera.keys.down = false;
                break;
            case Key::A:
                camera.keys.left = false;
                break;
            case Key::D:
                camera.keys.right = false;
                break;
            case Key::Shift:
                camera.movementSpeed = speed;
                break;
            }
        }

        void OnMouseMove(int32_t x, int32_t y, MouseButtons buttons) override
        {
            int32_t dx = static_cast<int32_t>(mouse_pos.x()) - x;
            int32_t dy = static_cast<int32_t>(mouse_pos.y()) - y;

            if (buttons & MouseButtons::Left)
            {
                camera.rotate(QVector3D(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
                view_updated = true;
            }
            if (buttons & MouseButtons::Right)
            {
                camera.translate(QVector3D(-0.0f, 0.0f, dy * .005f));
                view_updated = true;
            }
            if (buttons & MouseButtons::Middle)
            {
                camera.translate(QVector3D(-dx * 0.01f, -dy * 0.01f, 0.0f));
                view_updated = true;
            }
            mouse_pos.setX(x);
            mouse_pos.setY(y);
        }

        bool ObjectVisible(const BBox& bbox) const override
        {
            return frustum.checkBox(AAbox(bbox));
        }

        void BeforeRender() override
        {
            start_tp = std::chrono::high_resolution_clock::now();
            if (view_updated)
            {
                view_updated = false;
                mvp = camera.matrices.perspective * camera.matrices.view;
                frustum.update(mvp);
            }
        }

        virtual void Push(QVulkanDeviceFunctions& funcs, VkCommandBuffer cmd_buf, VkPipelineLayout layout) const
        {
            funcs.vkCmdPushConstants(
                cmd_buf,
                layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                mvp_layout.GetOffset(),
                mvp_layout.GetSize(),
                mvp.constData()
            );
        }

        std::string GetInfoString() const
        {
            std::string res;
            res += " - " + std::to_string(frame_counter) + " fps";
            res += " - " + std::to_string(camera.viewPos.x()) + "; "
                + std::to_string(camera.viewPos.y()) + "; "
                + std::to_string(camera.viewPos.z()) + "; ";

            res += " - " + std::to_string(camera.rotation.x()) + "; "
                + std::to_string(camera.rotation.y()) + "; "
                + std::to_string(camera.rotation.z()) + "; ";

            return res;
        }

        void AfterRender() override
        {
            frame_counter++;
            auto render_end = std::chrono::high_resolution_clock::now();
            auto time_diff = std::chrono::duration<double, std::milli>(render_end - start_tp).count();
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
            float fps_timer = static_cast<float>(std::chrono::duration<double, std::milli>(render_end - last_timestamp).count());
            if (fps_timer > 1000.0f)
            {
                lastFPS = static_cast<uint32_t>(static_cast<float>(frame_counter) * (1000.0f / fps_timer));

                info = GetInfoString();

                frame_counter = 0;
                last_timestamp = render_end;
            }
        }

        const std::string& GetInfo() const override
        {
            return info;
        }

        const IPushConstantLayout& GetMvpLayout() const override
        {
            return mvp_layout;
        }

        Vector3f GetViewPos() const override
        {
            return { camera.viewPos.x(), camera.viewPos.y(), camera.viewPos.z(), };
        }

        virtual ~Camera() = default;
    };

    std::unique_ptr<CameraRaii> CreateRaiiCamera()
    {
        return std::make_unique<Camera>();
    }

    std::unique_ptr<Vulkan::ICamera> CreateCamera()
    {
        return CreateRaiiCamera();
    }

    PushConstantLayout::PushConstantLayout(uint32_t offset, uint32_t size)
        : offset(offset)
        , size(size)
    {
    }

    uint32_t PushConstantLayout::GetSize() const
    {
        return size;
    }

    uint32_t PushConstantLayout::GetOffset() const
    {
        return offset;
    }
}
