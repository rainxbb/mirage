#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vulkan/vulkan.h>

namespace Mirage
{

class Window
{
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    void PollEvents();
    bool ShouldClose() const;
    void RequestClose();
    void GetFramebufferSize(int& width, int& height) const;

    SDL_Window* GetSDLWindow() const { return m_window; }
    VkSurfaceKHR CreateVulkanSurface(VkInstance instance) const;

    bool IsKeyDown(SDL_Scancode key) const;
    bool IsMouseButtonDown(Uint8 button) const;
    void GetMousePosition(float& x, float& y) const;

private:
    SDL_Window* m_window = nullptr;
    bool m_shouldClose = false;
};

} // namespace Mirage
