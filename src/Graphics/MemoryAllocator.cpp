#include "MemoryAllocator.h"

#include <stdexcept>

namespace Mirage
{

MemoryAllocator::MemoryAllocator(std::shared_ptr<VulkanContext> context) : m_context(context) {}
MemoryAllocator::~MemoryAllocator() {}

uint32_t MemoryAllocator::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_context->GetPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type!");
}

void MemoryAllocator::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags properties, VkBuffer& buffer, Allocation& allocation)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_context->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }

    allocation = AllocateBuffer(size, usage, properties);
    vkBindBufferMemory(m_context->GetDevice(), buffer, allocation.memory, 0);
}

Allocation MemoryAllocator::AllocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                           VkMemoryPropertyFlags properties)
{
    // Note: In a real scenario, you'd query the buffer's memory requirements.
    // Here we assume the size passed is the required size for simplicity,
    // or we create a dummy buffer to get requirements.
    VkBuffer dummyBuffer;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(m_context->GetDevice(), &bufferInfo, nullptr, &dummyBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_context->GetDevice(), dummyBuffer, &memRequirements);
    vkDestroyBuffer(m_context->GetDevice(), dummyBuffer, nullptr);

    Allocation alloc;
    alloc.size = memRequirements.size;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_context->GetDevice(), &allocInfo, nullptr, &alloc.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        vkMapMemory(m_context->GetDevice(), alloc.memory, 0, alloc.size, 0, &alloc.mappedData);
    }

    return alloc;
}

Allocation MemoryAllocator::AllocateImage(VkImage image, VkMemoryPropertyFlags properties)
{
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_context->GetDevice(), image, &memRequirements);

    Allocation alloc;
    alloc.size = memRequirements.size;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_context->GetDevice(), &allocInfo, nullptr, &alloc.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(m_context->GetDevice(), image, alloc.memory, 0);
    return alloc;
}

void MemoryAllocator::Free(Allocation& allocation)
{
    if (allocation.mappedData)
    {
        vkUnmapMemory(m_context->GetDevice(), allocation.memory);
        allocation.mappedData = nullptr;
    }
    if (allocation.memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_context->GetDevice(), allocation.memory, nullptr);
        allocation.memory = VK_NULL_HANDLE;
    }
}

} // namespace Mirage
