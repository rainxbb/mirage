#pragma once
#include "../Graphics/VulkanContext.h"

#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Mirage
{

struct XrSwapchainData
{
    XrSwapchain handle;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    uint32_t width;
    uint32_t height;
    uint32_t arraySize;
};

struct XrViewData
{
    XrPosef pose;
    XrFovf fov;
    glm::mat4 projection;
    glm::mat4 view;
};

class OpenXRContext
{
public:
    OpenXRContext(std::shared_ptr<VulkanContext> context);
    ~OpenXRContext();

    void Initialize();
    void PollEvents();

    bool BeginFrame();
    void RenderViews(const std::function<void(uint32_t viewIndex, VkImageView colorView,
                                              VkImageView depthView, VkExtent2D extent)>& renderFunc);
    void EndFrame();

    bool IsSessionRunning() const { return m_sessionRunning; }
    bool IsSessionFocused() const { return m_sessionState == XR_SESSION_STATE_FOCUSED; }
    const std::vector<XrViewData>& GetViews() const { return m_views; }

private:
    void CreateSession();
    void CreateSwapchains();
    void CreateViewConfigurations();

    void HandleSessionStateChange(XrSessionState newState);

    std::shared_ptr<VulkanContext> m_context;

    XrInstance m_instance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSession m_session = XR_NULL_HANDLE;
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
    bool m_sessionRunning = false;

    XrSpace m_appSpace = XR_NULL_HANDLE;

    std::vector<XrViewData> m_views;
    std::vector<XrSwapchainData> m_swapchains;

    XrFrameState m_frameState{};
};

} // namespace Mirage
