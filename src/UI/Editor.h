#pragma once
#include "../Core/Window.h"
#include "../Graphics/BindlessAllocator.h"
#include "../Graphics/MemoryAllocator.h"
#include "../Graphics/VulkanContext.h"
#include "../Scene/Scene.h"

#include <glm/glm.hpp>
#include <memory>

namespace Mirage
{

class Editor
{
public:
    Editor(std::shared_ptr<VulkanContext> context, std::shared_ptr<Window> window, VkFormat swapchainFormat,
           std::shared_ptr<MemoryAllocator> allocator, std::shared_ptr<BindlessAllocator> bindlessAlloc);
    ~Editor();

    void NewFrame();
    void DrawUI(Scene& scene, const glm::mat4& view, const glm::mat4& proj, uint32_t viewportWidth,
                uint32_t viewportHeight, VkExtent2D& outViewportSize, VkDescriptorSet viewportTexture);
    void EndFrame();
    void RecordDrawData(VkCommandBuffer cmd);

    bool IsUIVisible() const { return m_showUI; }
    bool IsViewportHovered() const { return m_isViewportHovered; }
    VkExtent2D GetViewportSize() const { return m_viewportSize; }

private:
    void HandleInput();
    void DrawStatusBar(Scene& scene);
    void DrawSceneHierarchy(Scene& scene);
    void DrawInspector(Scene& scene);
    void DrawContentBrowser();
    void DrawSceneViewport(VkDescriptorSet viewportTexture, VkExtent2D& outViewportSize, Scene& scene,
                           const glm::mat4& view, const glm::mat4& proj);

    void LoadDroppedModel(Scene& scene, const std::string& path);

    std::shared_ptr<VulkanContext> m_context;
    std::shared_ptr<Window> m_window;
    std::shared_ptr<MemoryAllocator> m_allocator;
    std::shared_ptr<BindlessAllocator> m_bindlessAlloc;
    VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;

    VkExtent2D m_viewportSize{0, 0};

    bool m_showUI = true;
    bool m_isViewportHovered = false;

    bool m_tabPressedLastFrame = false;
    bool m_wPressedLastFrame = false;
    bool m_ePressedLastFrame = false;
    bool m_rPressedLastFrame = false;

    int m_selectedEntityIndex = -1;
    int m_gizmoOperation = 0;
};

} // namespace Mirage
