#pragma once

#include <memory>
#include <cstdint>
#include <string>

namespace Vulkan
{

struct IPushConstantLayout;

enum class Key : uint32_t
{
    W = 0,
    S,
    A,
    D,
    Shift
};

enum MouseButtons : uint32_t
{
    None   = 0u,
    Left   = 1u << 0u,
    Right  = 1u << 1u,
    Middle = 1u << 2u,
};

struct BBox
{
    float min_x = 0.f;
    float min_y = 0.f;
    float min_z = 0.f;
    float max_x = 0.f;
    float max_y = 0.f;
    float max_z = 0.f;
};

struct ICamera
{
    virtual void SetPosition(float x, float y, float z) = 0;
    virtual void SetRotation(float x, float y, float z) = 0;
    virtual void SetPerspective(float fov, float aspect, float znear, float zfar) = 0;
    virtual void UpdateAspectRatio(float aspect) = 0;

    virtual void OnKeyPressed(Key key) = 0;
    virtual void OnKeyReleased(Key key) = 0;
    virtual void OnMouseMove(int32_t x, int32_t y, MouseButtons buttons) = 0;

    virtual bool ObjectVisible(const BBox& bbox) const = 0;

    virtual const IPushConstantLayout& GetMvpLayout() const = 0;
    virtual const std::string& GetInfo() const = 0;

    virtual ~ICamera() = default;
};

std::unique_ptr<ICamera> CreateCamera();

}
