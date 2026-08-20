#pragma once
#include "../Graphics/MemoryAllocator.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/Swapchain.h"
#include "../Graphics/VulkanContext.h"
#include "../Graphics/BindlessAllocator.h"
#include "../Scene/Scene.h"
#include "../UI/Editor.h"
#include "../VR/OpenXRContext.h"
#include "Window.h"

#include <memory>

namespace Mirage
{

class Application
{
public:
    Application();
    ~Application();

    void Run();

private:
    void Init();
    void LoadAssets();
    void Update();

    std::shared_ptr<Window> m_window;
    std::shared_ptr<VulkanContext> m_context;
    std::shared_ptr<Swapchain> m_swapchain;
    std::shared_ptr<MemoryAllocator> m_allocator;
    std::shared_ptr<BindlessAllocator> m_bindlessAlloc;
    std::shared_ptr<ShaderManager> m_shaderMgr;
    std::shared_ptr<OpenXRContext> m_xrContext;
    std::shared_ptr<Renderer> m_renderer;
    std::unique_ptr<Editor> m_editor;

    Scene m_scene;

    glm::vec3 m_camPos{0.0f, 1.0f, 3.0f};
    float m_yaw = -90.0f, m_pitch = 0.0f;
};

} // namespace Mirage
