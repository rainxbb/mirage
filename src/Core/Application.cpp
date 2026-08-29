#include "Application.h"

#include "../Assets/Mesh.h"
#include "../Assets/Model.h"
#include "../Assets/Texture.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

namespace Mirage
{

Application::Application(bool forceDesktop) : m_forceDesktop(forceDesktop) { Init(forceDesktop); }

Application::~Application()
{
    if (m_context)
    {
        m_context->WaitIdle();
    }

    m_renderer.reset();
    m_editor.reset();
    m_shaderMgr.reset();
    m_bindlessAlloc.reset();
    m_allocator.reset();
    m_swapchain.reset();
    m_xrContext.reset();

    if (m_context)
    {
        m_context.reset();
    }
    m_window.reset();
}

void Application::Init(bool forceDesktop)
{
    m_window = std::make_shared<Window>("Mirage", 1920, 1080);

    m_xrContext = std::make_shared<OpenXRContext>();
    std::vector<const char*> xrInstExts;
    std::vector<const char*> xrDevExts;

    if (!forceDesktop)
    {
        try
        {
            m_xrContext->CreateInstance();
            xrInstExts = m_xrContext->GetRequiredVulkanInstanceExtensions();
            xrDevExts = m_xrContext->GetRequiredVulkanDeviceExtensions();

            m_context = std::make_shared<VulkanContext>(m_window, xrInstExts, xrDevExts);
            VkPhysicalDevice xrPhysicalDevice =
                m_xrContext->GetRequiredVulkanPhysicalDevice(m_context->GetInstance());
            m_context->Initialize(xrPhysicalDevice);

            m_xrContext->CreateSession(m_context);
            m_xrContext->CreateReferenceSpace();
            m_xrContext->CreateViewConfigurations();
            m_xrContext->CreateSwapchains();

            std::cout << "OpenXR Initialized successfully.\n";
        }
        catch (const std::exception& e)
        {
            std::cerr << "OpenXR failed to initialize (Desktop-only): " << e.what() << "\n";
            m_xrContext = nullptr;

            std::vector<const char*> emptyExts;
            m_context = std::make_shared<VulkanContext>(m_window, emptyExts, emptyExts);
            m_context->Initialize();
        }
    }
    else
    {
        std::vector<const char*> emptyExts;
        m_context = std::make_shared<VulkanContext>(m_window, emptyExts, emptyExts);
        m_context->Initialize();
        m_xrContext = nullptr;
    }

    m_swapchain = std::make_shared<Swapchain>(m_context, m_window);
    m_allocator = std::make_shared<MemoryAllocator>(m_context);
    m_bindlessAlloc = std::make_shared<BindlessAllocator>(10000);
    m_shaderMgr = std::make_shared<ShaderManager>(m_context, "shaders");

    m_renderer = std::make_shared<Renderer>(m_context, m_swapchain, m_allocator, m_shaderMgr);
    m_renderer->InitPipeline();

    m_editor = std::make_unique<Editor>(m_context, m_window, m_swapchain->GetImageFormat(), m_allocator,
                                        m_bindlessAlloc);

    LoadAssets();
}

void Application::LoadAssets()
{
    int texW = 256, texH = 256;
    std::vector<uint8_t> texPixels(texW * texH * 4);
    for (int y = 0; y < texH; ++y)
    {
        for (int x = 0; x < texW; ++x)
        {
            int idx = (y * texW + x) * 4;
            bool white = ((x / 32) + (y / 32)) % 2 == 0;
            uint8_t val = white ? 255 : 50;
            texPixels[idx + 0] = val;
            texPixels[idx + 1] = val;
            texPixels[idx + 2] = val;
            texPixels[idx + 3] = 255;
        }
    }

    auto checkerTex =
        std::make_shared<Texture>(m_context, m_allocator, m_bindlessAlloc, texPixels.data(), texW, texH, 4);

    std::vector<Vertex> cubeVerts = {
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    };
    std::vector<uint32_t> cubeIndices = {0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,
                                         8,  9,  10, 10, 11, 8,  12, 13, 14, 14, 15, 12,
                                         16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

    auto cubeMesh = std::make_shared<Mesh>(m_context, m_allocator, cubeVerts, cubeIndices);

    std::string modelPath = "assets/models/DamagedHelmet/DamagedHelmet.gltf";

    auto helmetModel = std::make_shared<Model>(m_context, m_allocator, m_bindlessAlloc, modelPath);

    Entity e1;
    e1.name = "Cube";
    e1.transform.position = glm::vec3(0.0f, 0.0f, -2.0f);
    e1.mesh = cubeMesh;
    e1.albedoTexture = checkerTex;
    m_scene.AddEntity(e1);

    Entity e2;
    e2.name = "Floor";
    e2.transform.position = glm::vec3(0.0f, -1.0f, 0.0f);
    e2.transform.scale = glm::vec3(10.0f, 0.1f, 10.0f);
    e2.mesh = cubeMesh;
    e2.albedoTexture = checkerTex;
    m_scene.AddEntity(e2);

    Entity e3;
    e3.name = "Helmet";
    e3.transform.position = glm::vec3(2.0f, 1.0f, -2.0f);
    e3.transform.scale = glm::vec3(0.7f);
    e3.transform.rotation = glm::quat(0.0f, 0.0f, 1.0f, 0.0f);
    e3.mesh = helmetModel->GetMeshes().front();
    e3.albedoTexture = helmetModel->GetMeshes().front()->GetTexture();
    m_scene.AddEntity(e3);
}

void Application::Update(float dt)
{
    if (m_window->IsKeyDown(SDL_SCANCODE_ESCAPE))
        m_window->RequestClose();

    bool isViewportHovered = m_editor->IsViewportHovered();
    bool rightMouseDown = m_window->IsMouseButtonDown(SDL_BUTTON_RIGHT);

    glm::vec3 front;
    front.x = cos(glm::radians(m_camYaw)) * cos(glm::radians(m_camPitch));
    front.y = sin(glm::radians(m_camPitch));
    front.z = sin(glm::radians(m_camYaw)) * cos(glm::radians(m_camPitch));
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    if (rightMouseDown && isViewportHovered)
    {
        if (!m_isRightMouseDown)
        {
            SDL_SetWindowRelativeMouseMode(m_window->GetSDLWindow(), true);
            SDL_SetWindowMouseGrab(m_window->GetSDLWindow(), true);
            SDL_GetRelativeMouseState(nullptr, nullptr);
        }
        m_isRightMouseDown = true;

        float deltaX = 0.0f, deltaY = 0.0f;
        SDL_GetRelativeMouseState(&deltaX, &deltaY);

        deltaX = std::clamp(deltaX, -50.0f, 50.0f);
        deltaY = std::clamp(deltaY, -50.0f, 50.0f);

        float sensitivity = 0.15f;
        m_camYaw += deltaX * sensitivity;
        m_camPitch -= deltaY * sensitivity;
        m_camPitch = std::clamp(m_camPitch, -89.0f, 89.0f);

        float speed = m_camSpeed * dt;
        if (m_window->IsKeyDown(SDL_SCANCODE_LSHIFT))
            speed *= 3.0f;

        if (m_window->IsKeyDown(SDL_SCANCODE_W))
            m_camPos += front * speed;
        if (m_window->IsKeyDown(SDL_SCANCODE_S))
            m_camPos -= front * speed;
        if (m_window->IsKeyDown(SDL_SCANCODE_A))
            m_camPos -= right * speed;
        if (m_window->IsKeyDown(SDL_SCANCODE_D))
            m_camPos += right * speed;
        if (m_window->IsKeyDown(SDL_SCANCODE_SPACE))
            m_camPos += up * speed;
        if (m_window->IsKeyDown(SDL_SCANCODE_LCTRL))
            m_camPos -= up * speed;
    }
    else
    {
        if (m_isRightMouseDown)
        {
            SDL_SetWindowRelativeMouseMode(m_window->GetSDLWindow(), false);
            SDL_SetWindowMouseGrab(m_window->GetSDLWindow(), false);
        }
        m_isRightMouseDown = false;
    }

    glm::mat4 view = glm::lookAt(m_camPos, m_camPos + front, up);

    VkExtent2D viewportSize = m_editor->GetViewportSize();
    if (viewportSize.width == 0 || viewportSize.height == 0)
    {
        viewportSize = m_swapchain->GetExtent();
    }
    float aspect = (float)viewportSize.width / (float)viewportSize.height;
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    proj[1][1] *= -1.0f;

    bool isVrMirror = !m_editor->IsUIVisible() && m_xrContext && m_xrContext->IsSessionRunning();

    if (m_xrContext && m_xrContext->IsSessionRunning())
    {
        if (m_xrContext->BeginFrame())
        {
            m_xrContext->RenderViews(
                [&](uint32_t viewIndex, VkImageView colorView, VkImage colorImage, VkImageView depthView,
                    VkImage depthImage, VkExtent2D extent)
                {
                    auto& xrViews = m_xrContext->GetViews();

                    if (isVrMirror && viewIndex == 0)
                    {
                        view = xrViews[0].view;
                        proj = proj;
                    }

                    m_renderer->RenderVR(m_scene, viewIndex, colorView, colorImage, depthView, depthImage,
                                         extent, xrViews[viewIndex].view, xrViews[viewIndex].projection,
                                         m_camPos);
                });
            m_xrContext->EndFrame();
        }
    }

    m_editor->NewFrame();
    m_editor->DrawUI(m_scene, view, proj, m_swapchain->GetExtent().width, m_swapchain->GetExtent().height,
                     viewportSize, m_renderer->GetViewportDescriptorSet());

    if (viewportSize.width > 0 && viewportSize.height > 0)
    {
        m_renderer->ResizeViewportTarget(viewportSize);
    }

    m_editor->EndFrame();

    m_renderer->RenderDesktop(m_scene, view, proj, m_camPos, *m_editor);
}

void Application::Run()
{
    uint64_t lastTime = SDL_GetPerformanceCounter();
    double freq = (double)SDL_GetPerformanceFrequency();

    while (!m_window->ShouldClose())
    {
        uint64_t currentTime = SDL_GetPerformanceCounter();
        float dt = (float)((currentTime - lastTime) / freq);
        lastTime = currentTime;
        dt = std::min(dt, 0.1f);

        m_window->PollEvents();
        if (m_xrContext)
            m_xrContext->PollEvents();
        Update(dt);
    }
}

} // namespace Mirage
