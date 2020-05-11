#include <QVulkanFunctions>

#include "ICamera.h"
#include "IRenderer.h"

namespace Vulkan
{
    class PushConstantLayout
        : public IPushConstantLayout
    {
    public:
        PushConstantLayout(uint32_t offset, uint32_t size);
        ~PushConstantLayout() override = default;
    
        uint32_t GetSize() const;
        uint32_t GetOffset() const;
    
    private:
        uint32_t size   = 0u;
        uint32_t offset = 0u;
    };

    struct CameraRaii
        : public ICamera
    {
        virtual void BeforeRender() = 0;
        virtual void Push(QVulkanDeviceFunctions& funcs, VkCommandBuffer cmd_buf, VkPipelineLayout layout) const = 0;
        virtual void AfterRender() = 0;
        virtual ~CameraRaii() = default;
    };

    std::unique_ptr<CameraRaii> CreateRaiiCamera();
}