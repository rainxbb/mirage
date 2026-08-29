#pragma once
#include "../Graphics/VulkanContext.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#define XR_USE_GRAPHICS_API_VULKAN
#define XR_EXTENSION_PROTOTYPES
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
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

class OpenXRContext {
public:
    OpenXRContext();
    ~OpenXRContext();

    void CreateInstance();
    
    std::vector<const char*> GetRequiredVulkanInstanceExtensions();
    std::vector<const char*> GetRequiredVulkanDeviceExtensions();
    VkPhysicalDevice GetRequiredVulkanPhysicalDevice(VkInstance vkInstance);

    void CreateSession(std::shared_ptr<VulkanContext> context);
    void CreateReferenceSpace();
    void CreateViewConfigurations();
    void CreateSwapchains();

    void PollEvents();
    bool BeginFrame();
    void RenderViews(const std::function<void(uint32_t viewIndex, VkImageView colorView, VkImage colorImage, VkImageView depthView, VkImage depthImage, VkExtent2D extent)>& renderFunc);
    void EndFrame();

    bool IsSessionRunning() const { return m_sessionRunning; }
    const std::vector<XrViewData>& GetViews() const { return m_views; }

private:
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

    std::vector<std::string> m_instanceExtensionStrings;
    std::vector<std::string> m_deviceExtensionStrings;
};

} // namespace Mirage
