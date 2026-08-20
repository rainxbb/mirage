#pragma once
#include "VulkanContext.h"

#include <vulkan/vulkan.h>

namespace Mirage
{

struct Allocation
{
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mappedData = nullptr;
    VkDeviceSize size = 0;
};

class MemoryAllocator
{
public:
    MemoryAllocator(std::shared_ptr<VulkanContext> context);
    ~MemoryAllocator();

    Allocation AllocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    Allocation AllocateImage(VkImage image, VkMemoryPropertyFlags properties);

    void Free(Allocation& allocation);

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, Allocation& allocation);

private:
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    std::shared_ptr<VulkanContext> m_context;
};

} // namespace Mirage
