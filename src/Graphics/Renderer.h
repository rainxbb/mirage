#pragma once
#include "../Scene/Scene.h"
#include "MemoryAllocator.h"
#include "ShaderManager.h"
#include "Swapchain.h"
#include "VulkanContext.h"

#include <array>
#include <glm/glm.hpp>
#include <vector>

namespace Mirage
{
class Editor;

struct FrameContext
{
    VkCommandBuffer desktopCmd = VK_NULL_HANDLE;
    VkCommandBuffer vrCmd = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSem = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSem = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    VkImage vrDepthImage = VK_NULL_HANDLE;
    Allocation vrDepthAllocation;
    VkImageView vrDepthImageView = VK_NULL_HANDLE;
    VkExtent2D vrDepthExtent{0, 0};
};

class Renderer
{
public:
    Renderer(std::shared_ptr<VulkanContext> context, std::shared_ptr<Swapchain> swapchain,
             std::shared_ptr<MemoryAllocator> allocator, std::shared_ptr<ShaderManager> shaderMgr);
    ~Renderer();

    void InitPipeline();
    void MarkPipelinesDirty();

    void RenderDesktop(const Scene& scene, const glm::mat4& view, const glm::mat4& proj,
                       const glm::vec3& camPos, Editor& editor);

    void RenderVR(const Scene& scene, uint32_t viewIndex, VkImageView colorView, VkImage colorImage,
                  VkImageView depthView, VkImage depthImage, VkExtent2D extent, const glm::mat4& view,
                  const glm::mat4& proj, const glm::vec3& camPos);

private:
    void CreateDesktopDepthResources(VkExtent2D extent);
    void CreateVrDepthResources(FrameContext& frame, VkExtent2D extent);
    void CreateSyncObjects();
    void RebuildPipeline();

    void RecordDesktopCommandBuffer(FrameContext& frame, VkImageView colorView, VkImage colorImage,
                                    VkExtent2D extent, const Scene& scene, const glm::mat4& view,
                                    const glm::mat4& proj, const glm::vec3& camPos, Editor& editor);

    void RecordVrCommandBuffer(FrameContext& frame, VkImageView colorView, VkImage colorImage,
                               VkImageView depthView, VkImage depthImage, VkExtent2D extent,
                               const Scene& scene, const glm::mat4& view, const glm::mat4& proj,
                               const glm::vec3& camPos);

    std::shared_ptr<VulkanContext> m_context;
    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<MemoryAllocator> m_allocator;
    std::shared_ptr<ShaderManager> m_shaderMgr;

    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    bool m_pipelineDirty = true;

    VkImage m_desktopDepthImage = VK_NULL_HANDLE;
    Allocation m_desktopDepthAllocation;
    VkImageView m_desktopDepthImageView = VK_NULL_HANDLE;
    VkExtent2D m_desktopDepthExtent{0, 0};

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> m_frames;
    uint32_t m_currentFrame = 0;
};

} // namespace Mirage
