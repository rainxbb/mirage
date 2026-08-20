#pragma once
#include "VulkanContext.h"

#include <memory>
#include <vector>

namespace Mirage
{

class Window;

struct SwapchainImage
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
};

class Swapchain
{
public:
    Swapchain(std::shared_ptr<VulkanContext> context, std::shared_ptr<Window> window);
    ~Swapchain();

    void Recreate();

    VkResult AcquireNextImage(VkSemaphore semaphore, uint32_t& imageIndex);
    VkResult Present(VkSemaphore semaphore, uint32_t imageIndex);

    VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
    VkFormat GetImageFormat() const { return m_imageFormat; }
    VkExtent2D GetExtent() const { return m_extent; }
    const std::vector<SwapchainImage>& GetImages() const { return m_images; }

private:
    void CreateSwapchain(VkExtent2D extent);
    void CreateImageViews();
    void Cleanup();

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    std::shared_ptr<VulkanContext> m_context;
    std::shared_ptr<Window> m_window;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_imageFormat;
    VkExtent2D m_extent;
    std::vector<SwapchainImage> m_images;
};

} // namespace Mirage
