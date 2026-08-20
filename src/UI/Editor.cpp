#include "Editor.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>

namespace Mirage
{

Editor::Editor(std::shared_ptr<VulkanContext> context, std::shared_ptr<Window> window,
               VkFormat swapchainFormat)
    : m_context(context), m_window(window)
{

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

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

void Editor::NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void Editor::DrawUI()
{
    ImGui::Begin("Mirage");
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void Editor::RecordDrawData(VkCommandBuffer cmd)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void Editor::EndFrame() {}

} // namespace Mirage
