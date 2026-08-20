#pragma once
#include "../Core/Window.h"
#include "../Graphics/VulkanContext.h"

#include <memory>

namespace Mirage
{

class Editor
{
public:
    Editor(std::shared_ptr<VulkanContext> context, std::shared_ptr<Window> window, VkFormat swapchainFormat);
    ~Editor();

    void NewFrame();
    void DrawUI();
    void RecordDrawData(VkCommandBuffer cmd);
    void EndFrame();

private:
    std::shared_ptr<VulkanContext> m_context;
    std::shared_ptr<Window> m_window;
    VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
};

} // namespace Mirage
