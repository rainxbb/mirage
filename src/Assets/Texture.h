#pragma once
#include "../Graphics/BindlessAllocator.h"
#include "../Graphics/MemoryAllocator.h"
#include "../Graphics/VulkanContext.h"

#include <memory>
#include <string>

namespace Mirage
{

class Texture
{
public:
    Texture(std::shared_ptr<VulkanContext> context, std::shared_ptr<MemoryAllocator> allocator,
            std::shared_ptr<BindlessAllocator> bindlessAlloc, const std::string& path);
    Texture(std::shared_ptr<VulkanContext> context, std::shared_ptr<MemoryAllocator> allocator,
            std::shared_ptr<BindlessAllocator> bindlessAlloc, const uint8_t* pixels, int width, int height,
            int channels);
    ~Texture();

    VkImageView GetImageView() const { return m_imageView; }
    VkSampler GetSampler() const { return m_sampler; }
    uint32_t GetBindlessIndex() const { return m_bindlessIndex; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    void CreateImageImage(int width, int height, VkFormat format, VkImageTiling tiling,
                          VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                          Allocation& allocation);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                               VkImageLayout newLayout);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void UpdateBindlessDescriptors();

    std::shared_ptr<VulkanContext> m_context;
    std::shared_ptr<MemoryAllocator> m_allocator;

    VkImage m_image = VK_NULL_HANDLE;
    Allocation m_imageAllocation;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    uint32_t m_bindlessIndex = 0;
    int m_width = 0, m_height = 0;
};

} // namespace Mirage
