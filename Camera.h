#pragma once
#include <QMatrix4x4>
#include <qmath.h>
#include <array>

class AAbox
{
    QVector3D minp;
    QVector3D maxp;
public:
    AAbox(QVector3D minp, QVector3D maxp)
        : minp(std::move(minp))
        , maxp(std::move(maxp))
    {
    }

    QVector3D GetPositive(const QVector4D& normal) const
    {
        auto p = minp;
        if (normal.x() >= 0)
            p.setX(maxp.x());
        if (normal.y() >= 0)
            p.setY(maxp.y());
        if (normal.z() >= 0)
            p.setZ(maxp.z());
        return p;
    }

    QVector3D GetNegative(const QVector4D& normal) const
    {
        auto p = maxp;
        if (normal.x() >= 0)
            p.setX(minp.x());
        if (normal.y() >= 0)
            p.setY(minp.y());
        if (normal.z() >= 0)
            p.setZ(minp.z());
        return p;
    }
};

class Frustum
{
public:
    enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };
    std::array<QVector4D, 6> planes;

    void update(QMatrix4x4 model)
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

    bool checkBox(const AAbox& box)
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
