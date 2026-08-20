#include "Application.h"

#include "../Assets/Mesh.h"
#include "../Assets/Texture.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

namespace Mirage
{

void CreateProceduralCube(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    vertices = {{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}};
    indices = {0, 1, 2, 2, 3, 0};
}

void CreateCheckerboardTexture(std::vector<uint8_t>& pixels, int& width, int& height)
{
    width = 256;
    height = 256;
    pixels.resize(width * height * 4);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = (y * width + x) * 4;
            bool white = ((x / 32) + (y / 32)) % 2 == 0;
            uint8_t val = white ? 255 : 50;
            pixels[idx + 0] = val;
            pixels[idx + 1] = val;
            pixels[idx + 2] = val;
            pixels[idx + 3] = 255;
        }
    }
}

Application::Application() { Init(); }

Application::~Application()
{
    m_context->WaitIdle();
}

void Application::Init()
{
    m_window = std::make_shared<Window>("Mirage", 1920, 1080);
    m_context = std::make_shared<VulkanContext>(m_window);
    m_swapchain = std::make_shared<Swapchain>(m_context, m_window);
    m_allocator = std::make_shared<MemoryAllocator>(m_context);
    m_bindlessAlloc = std::make_shared<BindlessAllocator>(10000);
    m_shaderMgr = std::make_shared<ShaderManager>(m_context, "shaders");

    try
    {
        m_xrContext = std::make_shared<OpenXRContext>(m_context);
        m_xrContext->Initialize();
        std::cout << "OpenXR Initialized successfully.\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "OpenXR failed to initialize (running in Desktop-only mode): " << e.what() << "\n";
        m_xrContext = nullptr;
    }

    m_renderer = std::make_shared<Renderer>(m_context, m_swapchain, m_allocator, m_shaderMgr);
    m_renderer->InitPipeline();

    m_editor = std::make_unique<Editor>(m_context, m_window, m_swapchain->GetImageFormat());

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
        // Front face
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        // Back face
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        // Top face
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        // Bottom face
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        // Right face
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        // Left face
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    };
    std::vector<uint32_t> cubeIndices = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9,  10, 10, 11, 8,  // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    auto cubeMesh = std::make_shared<Mesh>(m_context, m_allocator, cubeVerts, cubeIndices);

    Entity e1;
    e1.name = "Procedural Cube";
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
}

void Application::Update()
{
    float speed = 0.05f;
    if (m_window->IsKeyDown(SDL_SCANCODE_W))
        m_camPos += glm::vec3(0.0f, 0.0f, -speed);
    if (m_window->IsKeyDown(SDL_SCANCODE_S))
        m_camPos += glm::vec3(0.0f, 0.0f, speed);
    if (m_window->IsKeyDown(SDL_SCANCODE_A))
        m_camPos += glm::vec3(-speed, 0.0f, 0.0f);
    if (m_window->IsKeyDown(SDL_SCANCODE_D))
        m_camPos += glm::vec3(speed, 0.0f, 0.0f);
    if (m_window->IsKeyDown(SDL_SCANCODE_ESCAPE))
        m_window->RequestClose();

    glm::mat4 view =
        glm::lookAt(m_camPos, m_camPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                      (float)m_swapchain->GetExtent().width / m_swapchain->GetExtent().height,
                                      0.1f, 100.0f);
    proj[1][1] *= -1;

    if (m_xrContext && m_xrContext->IsSessionRunning())
    {
        if (m_xrContext->BeginFrame())
        {
            m_xrContext->RenderViews(
                [&](uint32_t viewIndex, VkImageView colorView, VkImage colorImage, VkImageView depthView,
                    VkImage depthImage, VkExtent2D extent)
                {
                    auto& xrView = m_xrContext->GetViews()[viewIndex];
                    m_renderer->RenderVR(m_scene, viewIndex, colorView, colorImage, depthView, depthImage,
                                         extent, xrView.view, xrView.projection, m_camPos);
                });
            m_xrContext->EndFrame();
        }
    }

    m_editor->NewFrame();
    m_editor->DrawUI();
    m_renderer->RenderDesktop(m_scene, view, proj, m_camPos, *m_editor);
}

void Application::Run()
{
    while (!m_window->ShouldClose())
    {
        m_window->PollEvents();
        if (m_xrContext)
            m_xrContext->PollEvents();
        Update();
    }
}

} // namespace Mirage
