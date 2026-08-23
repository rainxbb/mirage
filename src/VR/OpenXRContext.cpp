#include "OpenXRContext.h"
#define _CRT_SECURE_NO_WARNINGS
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

#define XR_CHECK(cmd)                                                                                        \
    do                                                                                                       \
    {                                                                                                        \
        XrResult res = cmd;                                                                                  \
        if (XR_FAILED(res))                                                                                  \
        {                                                                                                    \
            char errorBuffer[XR_MAX_RESULT_STRING_SIZE] = "Unknown";                                         \
            if (m_instance != XR_NULL_HANDLE)                                                                \
            {                                                                                                \
                xrResultToString(m_instance, res, errorBuffer);                                              \
            }                                                                                                \
            throw std::runtime_error(std::string("OpenXR Error: ") + errorBuffer +                           \
                                     " (Code: " + std::to_string((int)res) + ") at " + #cmd);                \
        }                                                                                                    \
    } while (0)

namespace Mirage
{

namespace
{
glm::mat4 XrPoseToViewMatrix(const XrPosef& pose)
{
    glm::quat orientation(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
    glm::vec3 position(pose.position.x, pose.position.y, pose.position.z);
    glm::mat4 view = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(orientation);
    return glm::inverse(view);
}

glm::mat4 XrFovToProjectionMatrix(const XrFovf& fov, float nearZ, float farZ)
{
    float tanLeft = tanf(fov.angleLeft);
    float tanRight = tanf(fov.angleRight);
    float tanUp = tanf(fov.angleUp);
    float tanDown = tanf(fov.angleDown);
    float tanWidth = tanRight - tanLeft;
    float tanHeight = tanUp - tanDown;

    glm::mat4 proj(0.0f);

    proj[0][0] = 2.0f / tanWidth;

    proj[1][1] = -2.0f / tanHeight;

    proj[2][0] = (tanRight + tanLeft) / tanWidth;
    proj[2][1] = (tanUp + tanDown) / tanHeight;

    proj[2][2] = -farZ / (farZ - nearZ);
    proj[2][3] = -1.0f;
    proj[3][2] = -(farZ * nearZ) / (farZ - nearZ);

    return proj;
}
} // namespace

OpenXRContext::OpenXRContext() {}

OpenXRContext::~OpenXRContext()
{
    if (m_sessionRunning)
        xrRequestExitSession(m_session);
    for (auto& sc : m_swapchains)
    {
        for (auto view : sc.imageViews)
            vkDestroyImageView(m_context->GetDevice(), view, nullptr);
        xrDestroySwapchain(sc.handle);
    }
    if (m_appSpace)
        xrDestroySpace(m_appSpace);
    if (m_session)
        xrDestroySession(m_session);
    if (m_instance)
        xrDestroyInstance(m_instance);
}

void OpenXRContext::CreateInstance()
{
    XrApplicationInfo appInfo{};
    strncpy(appInfo.applicationName, "Mirage", XR_MAX_APPLICATION_NAME_SIZE - 1);
    appInfo.applicationVersion = 1;
    strncpy(appInfo.engineName, "Mirage", XR_MAX_ENGINE_NAME_SIZE - 1);
    appInfo.engineVersion = 1;
    appInfo.apiVersion = XR_API_VERSION_1_0;

    uint32_t extensionCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
    std::vector<XrExtensionProperties> extensionProperties(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount,
                                           extensionProperties.data());

    std::vector<const char*> enabledExtensions;
    for (const auto& prop : extensionProperties)
    {
        if (strcmp(prop.extensionName, XR_KHR_VULKAN_ENABLE_EXTENSION_NAME) == 0)
        {
            enabledExtensions.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);
        }
    }

    XrInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.createFlags = 0;
    instanceCreateInfo.applicationInfo = appInfo;
    instanceCreateInfo.enabledApiLayerCount = 0;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceCreateInfo.enabledExtensionNames = enabledExtensions.data();

    XR_CHECK(xrCreateInstance(&instanceCreateInfo, &m_instance));

    XrSystemGetInfo systemGetInfo{};
    systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XR_CHECK(xrGetSystem(m_instance, &systemGetInfo, &m_systemId));
}

std::vector<const char*> OpenXRContext::GetRequiredVulkanInstanceExtensions()
{
    auto xrGetVulkanInstanceExtensionsKHR = [&]()
    {
        PFN_xrGetVulkanInstanceExtensionsKHR func = nullptr;

        XR_CHECK(xrGetInstanceProcAddr(m_instance, "xrGetVulkanInstanceExtensionsKHR",
                                       reinterpret_cast<PFN_xrVoidFunction*>(&func)));

        return func;
    }();
    uint32_t bufferSize = 0;
    xrGetVulkanInstanceExtensionsKHR(m_instance, m_systemId, 0, &bufferSize, nullptr);
    std::vector<char> buffer(bufferSize);
    xrGetVulkanInstanceExtensionsKHR(m_instance, m_systemId, bufferSize, &bufferSize, buffer.data());

    std::string extensionsStr(buffer.data());
    m_instanceExtensionStrings.clear();
    std::stringstream ss(extensionsStr);
    std::string item;
    while (std::getline(ss, item, ' '))
    {
        if (!item.empty())
            m_instanceExtensionStrings.push_back(item);
    }

    std::vector<const char*> result;
    for (const auto& str : m_instanceExtensionStrings)
        result.push_back(str.c_str());
    return result;
}

std::vector<const char*> OpenXRContext::GetRequiredVulkanDeviceExtensions()
{
    auto xrGetVulkanDeviceExtensionsKHR = [&]()
    {
        PFN_xrGetVulkanDeviceExtensionsKHR func = nullptr;

        XR_CHECK(xrGetInstanceProcAddr(m_instance, "xrGetVulkanDeviceExtensionsKHR",
                                       reinterpret_cast<PFN_xrVoidFunction*>(&func)));

        return func;
    }();
    uint32_t bufferSize = 0;
    xrGetVulkanDeviceExtensionsKHR(m_instance, m_systemId, 0, &bufferSize, nullptr);
    std::vector<char> buffer(bufferSize);
    xrGetVulkanDeviceExtensionsKHR(m_instance, m_systemId, bufferSize, &bufferSize, buffer.data());

    std::string extensionsStr(buffer.data());
    m_deviceExtensionStrings.clear();
    std::stringstream ss(extensionsStr);
    std::string item;
    while (std::getline(ss, item, ' '))
    {
        if (!item.empty())
            m_deviceExtensionStrings.push_back(item);
    }

    std::vector<const char*> result;
    for (const auto& str : m_deviceExtensionStrings)
        result.push_back(str.c_str());
    return result;
}

VkPhysicalDevice OpenXRContext::GetRequiredVulkanPhysicalDevice(VkInstance vkInstance)
{
    auto xrGetVulkanGraphicsDeviceKHR = [&]()
    {
        PFN_xrGetVulkanGraphicsDeviceKHR func = nullptr;
        XR_CHECK(xrGetInstanceProcAddr(m_instance, "xrGetVulkanGraphicsDeviceKHR",
                                       reinterpret_cast<PFN_xrVoidFunction*>(&func)));
        return func;
    }();

    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    XR_CHECK(xrGetVulkanGraphicsDeviceKHR(m_instance, m_systemId, vkInstance, &vkPhysicalDevice));
    return vkPhysicalDevice;
}

void OpenXRContext::CreateSession(std::shared_ptr<VulkanContext> context)
{
    m_context = context;

    auto xrGetVulkanGraphicsRequirementsKHR = [&]()
    {
        PFN_xrGetVulkanGraphicsRequirementsKHR func = nullptr;
        XR_CHECK(xrGetInstanceProcAddr(m_instance, "xrGetVulkanGraphicsRequirementsKHR",
                                       reinterpret_cast<PFN_xrVoidFunction*>(&func)));
        return func;
    }();

    XrGraphicsRequirementsVulkanKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
    XR_CHECK(xrGetVulkanGraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements));

    XrGraphicsBindingVulkanKHR graphicsBinding{};
    graphicsBinding.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
    graphicsBinding.instance = m_context->GetInstance();
    graphicsBinding.physicalDevice = m_context->GetPhysicalDevice();
    graphicsBinding.device = m_context->GetDevice();
    graphicsBinding.queueFamilyIndex = m_context->GetQueueIndices().graphicsFamily;
    graphicsBinding.queueIndex = 0;

    XrSessionCreateInfo sessionCreateInfo{};
    sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionCreateInfo.next = &graphicsBinding;
    sessionCreateInfo.systemId = m_systemId;

    XR_CHECK(xrCreateSession(m_instance, &sessionCreateInfo, &m_session));
}

void OpenXRContext::CreateReferenceSpace()
{
    XrReferenceSpaceCreateInfo spaceCreateInfo{};
    spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    spaceCreateInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};
    XR_CHECK(xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_appSpace));
}

void OpenXRContext::CreateViewConfigurations()
{
    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0,
                                      &viewCount, nullptr);
    std::vector<XrViewConfigurationView> viewConfigs(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                      viewCount, &viewCount, viewConfigs.data());
    m_views.resize(viewCount);
}

void OpenXRContext::CreateSwapchains()
{
    uint32_t viewCount = static_cast<uint32_t>(m_views.size());
    m_swapchains.resize(viewCount);

    uint32_t formatCount = 0;
    xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr);
    std::vector<int64_t> formats(formatCount);
    xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount, formats.data());

    int64_t chosenFormat = VK_FORMAT_R8G8B8A8_UNORM;
    bool foundUnorm = false;
    for (int64_t fmt : formats)
    {
        if (fmt == VK_FORMAT_R8G8B8A8_UNORM)
        {
            chosenFormat = fmt;
            foundUnorm = true;
            break;
        }
    }

    if (!foundUnorm)
    {
        for (int64_t fmt : formats)
        {
            if (fmt == VK_FORMAT_R8G8B8A8_SRGB)
            {
                chosenFormat = fmt;
                break;
            }
        }
    }

    for (uint32_t i = 0; i < viewCount; i++)
    {
        uint32_t vcCount = 0;
        xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                          0, &vcCount, nullptr);
        std::vector<XrViewConfigurationView> vcs(vcCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                          vcCount, &vcCount, vcs.data());

        XrSwapchainCreateInfo swapchainCreateInfo{};
        swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        swapchainCreateInfo.usageFlags =
            XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        swapchainCreateInfo.format = chosenFormat;
        swapchainCreateInfo.sampleCount = 1;
        swapchainCreateInfo.width = vcs[i].recommendedImageRectWidth;
        swapchainCreateInfo.height = vcs[i].recommendedImageRectHeight;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.mipCount = 1;

        m_swapchains[i].width = swapchainCreateInfo.width;
        m_swapchains[i].height = swapchainCreateInfo.height;
        m_swapchains[i].arraySize = swapchainCreateInfo.arraySize;

        XR_CHECK(xrCreateSwapchain(m_session, &swapchainCreateInfo, &m_swapchains[i].handle));

        uint32_t imageCount;
        xrEnumerateSwapchainImages(m_swapchains[i].handle, 0, &imageCount, nullptr);

        std::vector<XrSwapchainImageVulkanKHR> images(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
        xrEnumerateSwapchainImages(m_swapchains[i].handle, imageCount, &imageCount,
                                   reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data()));

        m_swapchains[i].images.resize(imageCount);
        m_swapchains[i].imageViews.resize(imageCount);

        for (uint32_t j = 0; j < imageCount; j++)
        {
            m_swapchains[i].images[j] = images[j].image;

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = images[j].image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = static_cast<VkFormat>(chosenFormat);
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_context->GetDevice(), &viewInfo, nullptr,
                                  &m_swapchains[i].imageViews[j]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create OpenXR swapchain image view");
            }
        }
    }
}

void OpenXRContext::PollEvents()
{
    XrEventDataBuffer eventData{};
    eventData.type = XR_TYPE_EVENT_DATA_BUFFER;
    while (xrPollEvent(m_instance, &eventData) == XR_SUCCESS)
    {
        if (eventData.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
        {
            auto sessionEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(&eventData);
            HandleSessionStateChange(sessionEvent.state);
        }
        eventData.type = XR_TYPE_EVENT_DATA_BUFFER;
    }
}

void OpenXRContext::HandleSessionStateChange(XrSessionState newState)
{
    m_sessionState = newState;
    switch (newState)
    {
        case XR_SESSION_STATE_READY:
        {
            XrSessionBeginInfo beginInfo{};
            beginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
            beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            XR_CHECK(xrBeginSession(m_session, &beginInfo));
            m_sessionRunning = true;
            break;
        }
        case XR_SESSION_STATE_STOPPING:
        case XR_SESSION_STATE_EXITING:
        case XR_SESSION_STATE_LOSS_PENDING:
            XR_CHECK(xrEndSession(m_session));
            m_sessionRunning = false;
            break;
        default:
            break;
    }
}

bool OpenXRContext::BeginFrame()
{
    if (!m_sessionRunning)
        return false;
    XR_CHECK(xrWaitFrame(m_session, nullptr, &m_frameState));
    XR_CHECK(xrBeginFrame(m_session, nullptr));
    if (!m_frameState.shouldRender)
        return false;

    XrViewLocateInfo viewLocateInfo{};
    viewLocateInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = m_frameState.predictedDisplayTime;
    viewLocateInfo.space = m_appSpace;

    XrViewState viewState{};
    viewState.type = XR_TYPE_VIEW_STATE;

    uint32_t viewCount = static_cast<uint32_t>(m_views.size());
    std::vector<XrView> rawViews(viewCount, {XR_TYPE_VIEW});
    XR_CHECK(xrLocateViews(m_session, &viewLocateInfo, &viewState, viewCount, &viewCount, rawViews.data()));

    for (uint32_t i = 0; i < viewCount; i++)
    {
        m_views[i].pose = rawViews[i].pose;
        m_views[i].fov = rawViews[i].fov;
        m_views[i].view = XrPoseToViewMatrix(rawViews[i].pose);
        m_views[i].projection = XrFovToProjectionMatrix(rawViews[i].fov, 0.05f, 100.0f);
    }
    return true;
}

void OpenXRContext::RenderViews(
    const std::function<void(uint32_t viewIndex, VkImageView colorView, VkImage colorImage,
                             VkImageView depthView, VkImage depthImage, VkExtent2D extent)>& renderFunc)
{
    if (!m_sessionRunning || !m_frameState.shouldRender)
        return;

    for (uint32_t i = 0; i < m_swapchains.size(); i++)
    {
        uint32_t imageIndex;
        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        XR_CHECK(xrAcquireSwapchainImage(m_swapchains[i].handle, &acquireInfo, &imageIndex));

        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        XR_CHECK(xrWaitSwapchainImage(m_swapchains[i].handle, &waitInfo));

        renderFunc(i, m_swapchains[i].imageViews[imageIndex], m_swapchains[i].images[imageIndex],
                   VK_NULL_HANDLE, VK_NULL_HANDLE, {m_swapchains[i].width, m_swapchains[i].height});

        XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        XR_CHECK(xrReleaseSwapchainImage(m_swapchains[i].handle, &releaseInfo));
    }
}

void OpenXRContext::EndFrame()
{
    if (!m_sessionRunning || !m_frameState.shouldRender)
    {
        XrFrameEndInfo endInfo{};
        endInfo.type = XR_TYPE_FRAME_END_INFO;
        endInfo.displayTime = m_frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = 0;
        XR_CHECK(xrEndFrame(m_session, &endInfo));
        return;
    }

    std::vector<XrCompositionLayerProjectionView> projectionViews(m_views.size());
    for (uint32_t i = 0; i < m_views.size(); i++)
    {
        projectionViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projectionViews[i].pose = m_views[i].pose;
        projectionViews[i].fov = m_views[i].fov;
        projectionViews[i].subImage.swapchain = m_swapchains[i].handle;
        projectionViews[i].subImage.imageRect.offset = {0, 0};
        projectionViews[i].subImage.imageRect.extent = {static_cast<int32_t>(m_swapchains[i].width),
                                                        static_cast<int32_t>(m_swapchains[i].height)};
    }

    XrCompositionLayerProjection projectionLayer{};
    projectionLayer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    projectionLayer.space = m_appSpace;
    projectionLayer.viewCount = static_cast<uint32_t>(projectionViews.size());
    projectionLayer.views = projectionViews.data();

    const XrCompositionLayerBaseHeader* layersPtr[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)};

    XrFrameEndInfo endInfo{};
    endInfo.type = XR_TYPE_FRAME_END_INFO;
    endInfo.displayTime = m_frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    endInfo.layers = layersPtr;

    XR_CHECK(xrEndFrame(m_session, &endInfo));
}

} // namespace Mirage
