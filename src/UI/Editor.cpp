#include "Editor.h"

#include "../Assets/Model.h"
#include "imgui_internal.h"

#include <SDL3/SDL.h>
#define GLM_ENABLE_EXPERIMENTAL
#include "IconsFontAwesome6.h"

#include <ImGuizmo.h>
#include <filesystem>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <stdexcept>

namespace Mirage
{

Editor::Editor(std::shared_ptr<VulkanContext> context, std::shared_ptr<Window> window,
               VkFormat swapchainFormat, std::shared_ptr<MemoryAllocator> allocator,
               std::shared_ptr<BindlessAllocator> bindlessAlloc)
    : m_context(context), m_window(window), m_allocator(allocator), m_bindlessAlloc(bindlessAlloc)
{

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 16.0f);
    ImFontConfig fontConfig;
    fontConfig.MergeMode = true;
    fontConfig.GlyphMinAdvanceX = 14.0f;
    fontConfig.PixelSnapH = true;
    static const ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io.Fonts->AddFontFromFileTTF("assets/fonts/fa-solid-900.ttf", 16.0f, &fontConfig, iconRanges);

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    style.WindowRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.GrabRounding = 2.0f;

    colors[ImGuiCol_Text] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    colors[ImGuiCol_CheckboxSelectedBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.50f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.18f, 0.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.78f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.29f, 0.29f, 0.29f, 0.78f);
    colors[ImGuiCol_Separator] = ImVec4(0.29f, 0.29f, 0.29f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    colors[ImGuiCol_InputTextCursor] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_Tab] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.29f, 0.29f, 0.29f, 0.78f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.39f, 0.39f, 0.39f, 0.78f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.69f, 0.69f, 0.69f, 0.78f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.29f, 0.29f, 0.29f, 0.50f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = ImVec4(0.29f, 0.50f, 1.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_TreeLines] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_DragDropTargetBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_UnsavedMarker] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    ImGui_ImplSDL3_InitForVulkan(window->GetSDLWindow());

    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    if (vkCreateDescriptorPool(m_context->GetDevice(), &pool_info, nullptr, &m_imguiPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create ImGui descriptor pool");
    }

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_context->GetInstance();
    init_info.PhysicalDevice = m_context->GetPhysicalDevice();
    init_info.Device = m_context->GetDevice();
    init_info.QueueFamily = m_context->GetQueueIndices().graphicsFamily;
    init_info.Queue = m_context->GetGraphicsQueue();
    init_info.DescriptorPool = m_imguiPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.UseDynamicRendering = true;

    VkPipelineRenderingCreateInfo pipeline_rendering_info = {};
    pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &swapchainFormat;
    pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = pipeline_rendering_info;

    ImGui_ImplVulkan_Init(&init_info);
}

Editor::~Editor()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (m_imguiPool)
        vkDestroyDescriptorPool(m_context->GetDevice(), m_imguiPool, nullptr);
}

void Editor::HandleInput()
{
    bool tabPressed = m_window->IsKeyDown(SDL_SCANCODE_TAB);
    if (tabPressed && !m_tabPressedLastFrame)
    {
        m_showUI = !m_showUI;
    }
    m_tabPressedLastFrame = tabPressed;

    if (m_showUI && !ImGui::GetIO().WantCaptureKeyboard)
    {
        bool wPressed = m_window->IsKeyDown(SDL_SCANCODE_W);
        if (wPressed && !m_wPressedLastFrame)
            m_gizmoOperation = 0;
        m_wPressedLastFrame = wPressed;

        bool ePressed = m_window->IsKeyDown(SDL_SCANCODE_E);
        if (ePressed && !m_ePressedLastFrame)
            m_gizmoOperation = 1;
        m_ePressedLastFrame = ePressed;

        bool rPressed = m_window->IsKeyDown(SDL_SCANCODE_R);
        if (rPressed && !m_rPressedLastFrame)
            m_gizmoOperation = 2;
        m_rPressedLastFrame = rPressed;
    }
}

void Editor::NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    HandleInput();
}

void Editor::DrawUI(Scene& scene, const glm::mat4& view, const glm::mat4& proj, uint32_t viewportWidth,
                    uint32_t viewportHeight, VkExtent2D& outViewportSize, VkDescriptorSet viewportTexture)
{
    if (!m_showUI)
        return;

    ImGuizmo::SetRect(0, 0, viewportWidth, viewportHeight);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    float statusBarHeight = ImGui::GetFrameHeight();
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - statusBarHeight));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    static bool firstTime = true;
    if (firstTime)
    {
        firstTime = false;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_left =
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
        ImGuiID dock_id_right =
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
        ImGuiID dock_id_bottom =
            ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left);
        ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
        ImGui::DockBuilderDockWindow("Content Browser", dock_id_bottom);
        ImGui::DockBuilderDockWindow("Scene Viewport", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::End();

    DrawSceneHierarchy(scene);
    DrawInspector(scene);
    DrawContentBrowser();
    DrawSceneViewport(viewportTexture, outViewportSize, scene, view, proj);
    DrawStatusBar(scene);

    if (m_selectedEntityIndex >= 0 && m_selectedEntityIndex < scene.GetEntities().size())
    {
        Entity& selectedEntity = scene.GetEntities()[m_selectedEntityIndex];

        ImGuizmo::MODE mode = ImGuizmo::LOCAL;
        ImGuizmo::OPERATION op = (ImGuizmo::OPERATION)m_gizmoOperation;

        glm::mat4 model = selectedEntity.transform.GetMatrix();

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), op, mode, glm::value_ptr(model)))
        {
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(model, selectedEntity.transform.scale, selectedEntity.transform.rotation,
                           selectedEntity.transform.position, skew, perspective);
        }
    }
}

void Editor::DrawSceneViewport(VkDescriptorSet viewportTexture, VkExtent2D& outViewportSize, Scene& scene,
                               const glm::mat4& view, const glm::mat4& proj)
{
    ImGui::Begin("Scene Viewport");

    m_isViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

    ImVec2 availSize = ImGui::GetContentRegionAvail();
    m_viewportSize.width = std::max(1u, (uint32_t)availSize.x);
    m_viewportSize.height = std::max(1u, (uint32_t)availSize.y);
    outViewportSize = m_viewportSize;

    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    if (viewportTexture && m_viewportSize.width > 0 && m_viewportSize.height > 0)
    {
        ImGui::Image(viewportTexture, ImVec2(m_viewportSize.width, m_viewportSize.height), ImVec2(0, 0),
                     ImVec2(1, 1));

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGuizmo::IsUsing() &&
            !ImGuizmo::IsOver())
        {
            m_selectedEntityIndex = -1;
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::SetRect(viewportPos.x, viewportPos.y, m_viewportSize.width, m_viewportSize.height);
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::AllowAxisFlip(true);

            if (m_selectedEntityIndex >= 0 && m_selectedEntityIndex < scene.GetEntities().size())
            {
                Entity& selectedEntity = scene.GetEntities()[m_selectedEntityIndex];

                ImGuizmo::OPERATION currentOp = ImGuizmo::TRANSLATE;
                if (m_gizmoOperation == 1)
                    currentOp = ImGuizmo::ROTATE;
                else if (m_gizmoOperation == 2)
                    currentOp = ImGuizmo::SCALE;

                ImGuizmo::MODE mode = ImGuizmo::LOCAL;
                glm::mat4 model = selectedEntity.transform.GetMatrix();

                glm::mat4 gizmoProj = proj;
                gizmoProj[1][1] *= -1.0f;

                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(gizmoProj), currentOp, mode,
                                         glm::value_ptr(model)))
                {
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    glm::decompose(model, selectedEntity.transform.scale, selectedEntity.transform.rotation,
                                   selectedEntity.transform.position, skew, perspective);
                }
            }
        }
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        int winX, winY;
        SDL_GetWindowPosition(m_window->GetSDLWindow(), &winX, &winY);

        SDL_Rect mouseRect;
        mouseRect.x = (int)(viewportPos.x - winX);
        mouseRect.y = (int)(viewportPos.y - winY);
        mouseRect.w = (int)availSize.x;
        mouseRect.h = (int)availSize.y;

        SDL_SetWindowMouseRect(m_window->GetSDLWindow(), &mouseRect);
    }
    else
    {
        SDL_SetWindowMouseRect(m_window->GetSDLWindow(), nullptr);
    }

    ImGui::End();
}

void Editor::DrawStatusBar(Scene& scene)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float barHeight = ImGui::GetFrameHeight();

    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - barHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, barHeight));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.13f, 0.13f, 0.13f, 1.0f));

    ImGui::Begin("##StatusBar", nullptr, window_flags);

    if (ImGui::BeginMenuBar())
    {
        ImGui::TextDisabled(ICON_FA_CHART_LINE " FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextDisabled(ICON_FA_CUBE " Entities: %zu", scene.GetEntities().size());

        float regionWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 buttonSize(32.0f, 24.0f);
        float totalWidth = buttonSize.x * 3.0f;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (regionWidth - totalWidth));

        ImVec2 startPos = ImGui::GetCursorScreenPos();
        ImVec2 endPos(startPos.x + totalWidth, startPos.y + buttonSize.y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec4 bgColor(0.18f, 0.18f, 0.18f, 1.0f);
        ImVec4 borderColor(0.08f, 0.08f, 0.08f, 1.0f);
        drawList->AddRectFilled(startPos, endPos, ImGui::GetColorU32(bgColor), 4.0f);
        drawList->AddRect(startPos, endPos, ImGui::GetColorU32(borderColor), 4.0f);

        const char* icons[] = {ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, ICON_FA_ROTATE, ICON_FA_EXPAND};
        const char* tooltips[] = {"Translate (W)", "Rotate (E)", "Scale (R)"};

        for (int i = 0; i < 3; i++)
        {
            ImVec2 btnStart(startPos.x + i * buttonSize.x, startPos.y);
            ImVec2 btnEnd(btnStart.x + buttonSize.x, btnStart.y + buttonSize.y);

            bool isHovered = ImGui::IsMouseHoveringRect(btnStart, btnEnd);
            bool isActive = (m_gizmoOperation == i);

            if (isActive)
            {
                drawList->AddRectFilled(btnStart, btnEnd,
                                        ImGui::GetColorU32(ImVec4(0.30f, 0.55f, 0.85f, 1.0f)), 0.0f);
            }
            else if (isHovered)
            {
                drawList->AddRectFilled(btnStart, btnEnd,
                                        ImGui::GetColorU32(ImVec4(0.25f, 0.25f, 0.25f, 1.0f)), 0.0f);
            }

            if (i > 0)
            {
                drawList->AddLine(ImVec2(btnStart.x, btnStart.y + 4.0f), ImVec2(btnStart.x, btnEnd.y - 4.0f),
                                  ImGui::GetColorU32(borderColor));
            }

            ImVec2 textSize = ImGui::CalcTextSize(icons[i]);
            ImVec2 textPos(btnStart.x + (buttonSize.x - textSize.x) * 0.5f,
                           btnStart.y + (buttonSize.y - textSize.y) * 0.5f);
            ImVec4 textColor = isActive ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
            drawList->AddText(textPos, ImGui::GetColorU32(textColor), icons[i]);

            ImGui::SetCursorScreenPos(btnStart);
            char btnId[16];
            snprintf(btnId, sizeof(btnId), "##gizmo%d", i);
            if (ImGui::InvisibleButton(btnId, buttonSize))
            {
                m_gizmoOperation = i;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", tooltips[i]);
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(endPos.x, startPos.y));

        ImGui::EndMenuBar();
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void Editor::DrawSceneHierarchy(Scene& scene)
{
    ImGui::Begin("Scene Hierarchy");

    static char searchBuffer[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##SearchHierarchy", ICON_FA_MAGNIFYING_GLASS " Search...", searchBuffer,
                             sizeof(searchBuffer));

    ImGui::Spacing();

    if (ImGui::Button(ICON_FA_PLUS " Add Entity", ImVec2(-1, 0)))
    {
        Entity newEntity;
        newEntity.name = "Entity_" + std::to_string(scene.GetEntities().size());
        scene.AddEntity(newEntity);
        m_selectedEntityIndex = static_cast<int>(scene.GetEntities().size()) - 1;
    }

    ImGui::Separator();
    ImGui::Spacing();

    auto& entities = scene.GetEntities();
    for (int i = 0; i < static_cast<int>(entities.size()); i++)
    {
        if (searchBuffer[0] != '\0')
        {
            if (entities[i].name.find(searchBuffer) == std::string::npos)
                continue;
        }

        bool isSelected = (m_selectedEntityIndex == i);

        if (isSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        ImGui::PushID(i);
        std::string displayName = std::string(ICON_FA_CUBE "  ") + entities[i].name;
        if (ImGui::Selectable(displayName.c_str(), isSelected))
        {
            m_selectedEntityIndex = i;
        }
        ImGui::PopID();

        if (isSelected)
        {
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
}

void Editor::DrawInspector(Scene& scene)
{
    ImGui::Begin("Inspector");

    if (m_selectedEntityIndex >= 0 && m_selectedEntityIndex < static_cast<int>(scene.GetEntities().size()))
    {
        Entity& entity = scene.GetEntities()[m_selectedEntityIndex];

        static char nameBuffer[128];
        strncpy(nameBuffer, entity.name.c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##EntityName", nameBuffer, sizeof(nameBuffer)))
        {
            entity.name = nameBuffer;
        }
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader((std::string(ICON_FA_CUBE "  Transform")).c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat3("Position", glm::value_ptr(entity.transform.position), 0.1f);

            glm::vec3 euler = glm::degrees(glm::eulerAngles(entity.transform.rotation));
            if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.5f))
            {
                entity.transform.rotation = glm::quat(glm::radians(euler));
            }

            ImGui::DragFloat3("Scale", glm::value_ptr(entity.transform.scale), 0.1f, 0.01f, 100.0f);
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader((std::string(ICON_FA_PALETTE "  Material")).c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit4("Albedo Tint", glm::value_ptr(entity.material.albedoTint));
            ImGui::SliderFloat("Metallic", &entity.material.metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &entity.material.roughness, 0.01f, 1.0f);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button(ICON_FA_PLUS " Add Component", ImVec2(-1, 0)))
        {
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.30f, 0.30f, 1.0f));
        if (ImGui::Button(ICON_FA_TRASH " Delete Entity", ImVec2(-1, 0)))
        {
            m_context->WaitIdle();
            scene.GetEntities().erase(scene.GetEntities().begin() + m_selectedEntityIndex);
            if (scene.GetEntities().empty())
                m_selectedEntityIndex = -1;
            else if (m_selectedEntityIndex >= static_cast<int>(scene.GetEntities().size()))
                m_selectedEntityIndex = static_cast<int>(scene.GetEntities().size()) - 1;
        }
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::TextDisabled(ICON_FA_CIRCLE_INFO " No entity selected.");
    }
    ImGui::End();
}

void Editor::DrawContentBrowser()
{
    ImGui::Begin("Content Browser");

    static char searchBuffer[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##SearchAssets", ICON_FA_MAGNIFYING_GLASS " Search assets...", searchBuffer,
                             sizeof(searchBuffer));

    ImGui::Spacing();

    namespace fs = std::filesystem;
    std::string assetsPath = "assets";

    if (fs::exists(assetsPath) && fs::is_directory(assetsPath))
    {
        for (const auto& entry : fs::recursive_directory_iterator(assetsPath))
        {
            if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();
                if (ext == ".gltf" || ext == ".glb" || ext == ".obj")
                {
                    std::string filename = entry.path().filename().string();

                    if (searchBuffer[0] != '\0')
                    {
                        if (filename.find(searchBuffer) == std::string::npos)
                            continue;
                    }

                    std::string fullPath = entry.path().string();
                    std::string displayName = std::string(ICON_FA_FILE_CODE "  ") + filename;

                    ImGui::PushID(fullPath.c_str());
                    if (ImGui::Selectable(displayName.c_str(), false))
                    {
                    }

                    if (ImGui::BeginDragDropSource())
                    {
                        ImGui::SetDragDropPayload("ASSET_MODEL", fullPath.c_str(), fullPath.length() + 1);
                        ImGui::Text("Dragging: %s", filename.c_str());
                        ImGui::EndDragDropSource();
                    }
                    ImGui::PopID();
                }
            }
        }
    }
    else
    {
        ImGui::TextDisabled(ICON_FA_TRIANGLE_EXCLAMATION " No 'assets' directory found.");
    }
    ImGui::End();
}

void Editor::LoadDroppedModel(Scene& scene, const std::string& path)
{
    try
    {
        auto model = std::make_shared<Model>(m_context, m_allocator, m_bindlessAlloc, path);
        std::string baseName = std::filesystem::path(path).stem().string();

        int partIndex = 0;
        for (const auto& mesh : model->GetMeshes())
        {
            Entity e;
            e.name = baseName + "_Part_" + std::to_string(partIndex++);
            e.transform.position = glm::vec3(0.0f, 0.0f, -3.0f);
            e.mesh = mesh;
            e.albedoTexture = mesh->GetTexture();
            scene.AddEntity(e);
        }
        m_selectedEntityIndex = scene.GetEntities().size() - 1;
        std::cout << "[Editor] Successfully spawned: " << baseName << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Editor] Failed to load dropped model: " << e.what() << "\n";
    }
}

void Editor::EndFrame() { ImGui::Render(); }

void Editor::RecordDrawData(VkCommandBuffer cmd)
{
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData && drawData->CmdListsCount > 0)
    {
        ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
    }
}

} // namespace Mirage
