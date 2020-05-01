#pragma once
#include <QMatrix4x4>
#include <qmath.h>

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
    bool flipY = false;

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
