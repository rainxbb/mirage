#include "Window.h"

#include "SDL3/SDL_vulkan.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <stdexcept>

namespace Mirage
{

Window::Window(const std::string& title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        throw std::runtime_error(std::string("Failed to initialize SDL3: ") + SDL_GetError());
    }

    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    m_window = SDL_CreateWindow(title.c_str(), width, height,
                                SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_window)
    {
        throw std::runtime_error(std::string("Failed to create SDL3 Window: ") + SDL_GetError());
    }
}

Window::~Window()
{
    if (m_window)
    {
        SDL_DestroyWindow(m_window);
    }
    SDL_Quit();
}

void Window::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT)
        {
            m_shouldClose = true;
        }
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
            event.window.windowID == SDL_GetWindowID(m_window))
        {
            m_shouldClose = true;
        }
    }
}

bool Window::ShouldClose() const { return m_shouldClose; }

void Window::RequestClose() { m_shouldClose = true; }

void Window::GetFramebufferSize(int& width, int& height) const
{
    SDL_GetWindowSizeInPixels(m_window, &width, &height);
}

VkSurfaceKHR Window::CreateVulkanSurface(VkInstance instance) const
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(m_window, instance, nullptr, &surface))
    {
        throw std::runtime_error(std::string("Failed to create Vulkan Surface: ") + SDL_GetError());
    }
    return surface;
}

bool Window::IsKeyDown(SDL_Scancode key) const
{
    const bool* state = SDL_GetKeyboardState(nullptr);
    return state[key];
}

bool Window::IsMouseButtonDown(Uint8 button) const
{
    return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(button)) != 0;
}

void Window::GetMousePosition(float& x, float& y) const { SDL_GetMouseState(&x, &y); }

} // namespace Mirage
