#include "Editor.h"

#include "../Assets/Model.h"
#include "imgui_internal.h"

#include <SDL3/SDL.h>
#define GLM_ENABLE_EXPERIMENTAL
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

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.95f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.10f, 0.80f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.50f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.65f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.65f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.75f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.65f, 0.95f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.65f, 0.95f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.40f, 0.65f, 0.95f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.65f, 0.95f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.65f, 0.95f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);

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
                    uint32_t viewportHeight)
{
    if (!m_showUI)
        return;

    ImGuizmo::SetRect(0, 0, viewportWidth, viewportHeight);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
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
        ImGui::DockBuilderDockWindow("Stats Overlay", dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::End();

    DrawMenuBar();
    DrawSceneHierarchy(scene);
    DrawInspector(scene);
    DrawContentBrowser();
    DrawStatsOverlay(scene);

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

void Editor::DrawMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                m_window->RequestClose();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Show Editor UI", "Tab", &m_showUI);
            ImGui::Separator();
            ImGui::Text("Gizmo Operation:");
            if (ImGui::RadioButton("Translate (W)", m_gizmoOperation == 0))
                m_gizmoOperation = 0;
            if (ImGui::RadioButton("Rotate (E)", m_gizmoOperation == 1))
                m_gizmoOperation = 1;
            if (ImGui::RadioButton("Scale (R)", m_gizmoOperation == 2))
                m_gizmoOperation = 2;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Editor::DrawSceneHierarchy(Scene& scene)
{
    ImGui::Begin("Scene Hierarchy");

    if (ImGui::Button("Add Entity", ImVec2(-1, 0)))
    {
        Entity newEntity;
        newEntity.name = "Entity_" + std::to_string(scene.GetEntities().size());
        scene.AddEntity(newEntity);
        m_selectedEntityIndex = scene.GetEntities().size() - 1;
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_MODEL"))
        {
            std::string path((const char*)payload->Data);
            LoadDroppedModel(scene, path);
        }
        ImGui::EndDragDropTarget();
    }

    auto& entities = scene.GetEntities();
    for (int i = 0; i < entities.size(); i++)
    {
        bool isSelected = (m_selectedEntityIndex == i);
        if (ImGui::Selectable(entities[i].name.c_str(), isSelected))
        {
            m_selectedEntityIndex = i;
        }
    }
    ImGui::End();
}

void Editor::DrawInspector(Scene& scene)
{
    ImGui::Begin("Inspector");

    if (m_selectedEntityIndex >= 0 && m_selectedEntityIndex < scene.GetEntities().size())
    {
        Entity& entity = scene.GetEntities()[m_selectedEntityIndex];

        ImGui::Text("Name: %s", entity.name.c_str());
        ImGui::Separator();

        ImGui::Text("Transform");
        ImGui::DragFloat3("Position", glm::value_ptr(entity.transform.position), 0.1f);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(entity.transform.rotation));
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.5f))
        {
            entity.transform.rotation = glm::quat(glm::radians(euler));
        }

        ImGui::DragFloat3("Scale", glm::value_ptr(entity.transform.scale), 0.1f, 0.01f, 100.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Material");
        ImGui::ColorEdit4("Albedo Tint", glm::value_ptr(entity.material.albedoTint));
        ImGui::SliderFloat("Metallic", &entity.material.metallic, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &entity.material.roughness, 0.01f, 1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.4f, 0.4f, 1.0f));
        if (ImGui::Button("Delete Entity", ImVec2(-1, 0)))
        {
            scene.GetEntities().erase(scene.GetEntities().begin() + m_selectedEntityIndex);
            if (scene.GetEntities().empty())
                m_selectedEntityIndex = -1;
            else if (m_selectedEntityIndex >= scene.GetEntities().size())
                m_selectedEntityIndex = scene.GetEntities().size() - 1;
        }
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::TextDisabled("No entity selected.");
    }
    ImGui::End();
}

void Editor::DrawStatsOverlay(Scene& scene)
{
    ImGui::Begin("Stats Overlay");
    ImGui::Text("FPS: %.1f (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("Entities: %zu", scene.GetEntities().size());
    ImGui::End();
}

void Editor::DrawContentBrowser()
{
    ImGui::Begin("Content Browser");

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
                    std::string fullPath = entry.path().string();

                    ImGui::Selectable(filename.c_str(), false);

                    if (ImGui::BeginDragDropSource())
                    {
                        ImGui::SetDragDropPayload("ASSET_MODEL", fullPath.c_str(), fullPath.length() + 1);
                        ImGui::Text("Dragging: %s", filename.c_str());
                        ImGui::EndDragDropSource();
                    }
                }
            }
        }
    }
    else
    {
        ImGui::TextDisabled("No 'assets' directory found.");
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
