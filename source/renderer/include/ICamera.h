#pragma once

#include <memory>

namespace Vulkan
{

struct ICamera
{
    virtual void SetPosition(float x, float y, float z) = 0;
    virtual void SetRotation(float x, float y, float z) = 0;
    virtual void SetPerspective(float fov, float aspect, float znear, float zfar) = 0;
    virtual void UpdateAspectRatio(float aspect) = 0;
    virtual ~ICamera() = default;
};

}

std::unique_ptr<Vulkan::ICamera> CreateCamera();
