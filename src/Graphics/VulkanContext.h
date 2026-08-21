#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>
#include <memory>
#include <string>
#include <vector>

namespace Mirage
{

class Window;

struct VulkanQueueIndices
{
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t presentFamily = UINT32_MAX;
    uint32_t computeFamily = UINT32_MAX;

    bool IsComplete() const { return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX; }
};

class VulkanContext
{
public:
    VulkanContext(std::shared_ptr<Window> window, const std::vector<const char*>& xrInstanceExtensions,
                  const std::vector<const char*>& xrDeviceExtensions);
    ~VulkanContext();

    void WaitIdle();
    void Initialize(VkPhysicalDevice forcedPhysicalDevice = VK_NULL_HANDLE);

    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkDevice GetDevice() const { return m_device; }
    VkSurfaceKHR GetSurface() const { return m_surface; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue GetPresentQueue() const { return m_presentQueue; }
    VkCommandPool GetCommandPool() const { return m_commandPool; }

    VkDescriptorSetLayout GetBindlessDescriptorSetLayout() const { return m_bindlessDescriptorSetLayout; }
    VkDescriptorPool GetBindlessDescriptorPool() const { return m_bindlessDescriptorPool; }
    VkDescriptorSet GetBindlessDescriptorSet() const { return m_bindlessDescriptorSet; }

    const VulkanQueueIndices& GetQueueIndices() const { return m_queueIndices; }
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    void CreateInstance();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();
    void CreateBindlessResources();

    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    VulkanQueueIndices FindQueueFamilies(VkPhysicalDevice device);

    std::shared_ptr<Window> m_window;

    std::vector<const char*> m_xrInstanceExtensions;
    std::vector<const char*> m_xrDeviceExtensions;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDevice m_forcedPhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VulkanQueueIndices m_queueIndices;

    VkDescriptorSetLayout m_bindlessDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_bindlessDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_bindlessDescriptorSet = VK_NULL_HANDLE;
};

} // namespace Mirage
