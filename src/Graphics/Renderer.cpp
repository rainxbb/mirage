#include "Renderer.h"

#include "../UI/Editor.h"
#include "Pipeline.h"

#include <array>
#include <iostream>
#include <stdexcept>

namespace Mirage
{

namespace
{

void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout,
                           VkImageLayout newLayout, bool isDepth)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        barrier.dstAccessMask = 0;
    }

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

} // namespace

Renderer::Renderer(std::shared_ptr<VulkanContext> context, std::shared_ptr<Swapchain> swapchain,
                   std::shared_ptr<MemoryAllocator> allocator, std::shared_ptr<ShaderManager> shaderMgr)
    : m_context(context), m_swapchain(swapchain), m_allocator(allocator), m_shaderMgr(shaderMgr)
{

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_context->GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT * 3;

    std::vector<VkCommandBuffer> cmds(MAX_FRAMES_IN_FLIGHT * 3);
    vkAllocateCommandBuffers(m_context->GetDevice(), &allocInfo, cmds.data());

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_frames[i].desktopCmd = cmds[i * 3];
        m_frames[i].vrCmds[0] = cmds[i * 3 + 1];
        m_frames[i].vrCmds[1] = cmds[i * 3 + 2];
    }

    CreateSyncObjects();
    CreateDesktopDepthResources(swapchain->GetExtent());

    m_shaderMgr->SetReloadCallback([this]() { MarkPipelinesDirty(); });
}

Renderer::~Renderer()
{
    if (!m_context)
        return;
    m_context->WaitIdle();

    if (m_pipeline)
        vkDestroyPipeline(m_context->GetDevice(), m_pipeline, nullptr);
    if (m_pipelineLayout)
        vkDestroyPipelineLayout(m_context->GetDevice(), m_pipelineLayout, nullptr);

    if (m_desktopDepthImageView)
        vkDestroyImageView(m_context->GetDevice(), m_desktopDepthImageView, nullptr);
    if (m_desktopDepthImage)
        vkDestroyImage(m_context->GetDevice(), m_desktopDepthImage, nullptr);
    m_allocator->Free(m_desktopDepthAllocation);

    std::vector<VkCommandBuffer> cmdsToFree;
    cmdsToFree.reserve(MAX_FRAMES_IN_FLIGHT * 3);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (m_frames[i].desktopCmd != VK_NULL_HANDLE)
            cmdsToFree.push_back(m_frames[i].desktopCmd);
        if (m_frames[i].vrCmds[0] != VK_NULL_HANDLE)
            cmdsToFree.push_back(m_frames[i].vrCmds[0]);
        if (m_frames[i].vrCmds[1] != VK_NULL_HANDLE)
            cmdsToFree.push_back(m_frames[i].vrCmds[1]);

        if (m_frames[i].imageAvailableSem != VK_NULL_HANDLE)
            vkDestroySemaphore(m_context->GetDevice(), m_frames[i].imageAvailableSem, nullptr);
        if (m_frames[i].renderFinishedSem != VK_NULL_HANDLE)
            vkDestroySemaphore(m_context->GetDevice(), m_frames[i].renderFinishedSem, nullptr);
        if (m_frames[i].inFlightFence != VK_NULL_HANDLE)
            vkDestroyFence(m_context->GetDevice(), m_frames[i].inFlightFence, nullptr);

        if (m_frames[i].vrDepthImageView != VK_NULL_HANDLE)
            vkDestroyImageView(m_context->GetDevice(), m_frames[i].vrDepthImageView, nullptr);
        if (m_frames[i].vrDepthImage != VK_NULL_HANDLE)
            vkDestroyImage(m_context->GetDevice(), m_frames[i].vrDepthImage, nullptr);
        m_allocator->Free(m_frames[i].vrDepthAllocation);
    }

    if (!cmdsToFree.empty())
    {
        vkFreeCommandBuffers(m_context->GetDevice(), m_context->GetCommandPool(),
                             static_cast<uint32_t>(cmdsToFree.size()), cmdsToFree.data());
    }
}

void Renderer::InitPipeline()
{
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkDescriptorSetLayout setLayout = m_context->GetBindlessDescriptorSetLayout();
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &setLayout;

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = 256;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    if (vkCreatePipelineLayout(m_context->GetDevice(), &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    RebuildPipeline();
}

void Renderer::MarkPipelinesDirty() { m_pipelineDirty = true; }

void Renderer::RebuildPipeline()
{
    m_context->WaitIdle();

    if (m_pipeline)
        vkDestroyPipeline(m_context->GetDevice(), m_pipeline, nullptr);

    VkShaderModule vertShader = m_shaderMgr->GetShader("pbr.vert.spv");
    VkShaderModule fragShader = m_shaderMgr->GetShader("pbr.frag.spv");

    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    PipelineBuilder builder(m_context);
    builder.SetShaders(vertShader, fragShader)
        .SetVertexInput(bindingDescriptions, attributeDescriptions)
        .SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetPolygonMode(VK_POLYGON_MODE_FILL)
        .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetMultisamplingNone()
        .EnableDepthTest(true, VK_COMPARE_OP_LESS)
        .DisableBlending()
        .SetColorAttachmentFormat(m_swapchain->GetImageFormat())
        .SetDepthFormat(VK_FORMAT_D32_SFLOAT);

    m_pipeline = builder.Build(m_pipelineLayout);
    m_pipelineDirty = false;
}

void Renderer::CreateDesktopDepthResources(VkExtent2D extent)
{
    if (m_desktopDepthExtent.width == extent.width && m_desktopDepthExtent.height == extent.height)
        return;

    if (m_desktopDepthImageView)
        vkDestroyImageView(m_context->GetDevice(), m_desktopDepthImageView, nullptr);
    if (m_desktopDepthImage)
        vkDestroyImage(m_context->GetDevice(), m_desktopDepthImage, nullptr);
    m_allocator->Free(m_desktopDepthAllocation);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_context->GetDevice(), &imageInfo, nullptr, &m_desktopDepthImage) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create depth image");
    }

    m_desktopDepthAllocation =
        m_allocator->AllocateImage(m_desktopDepthImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_desktopDepthExtent = extent;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_desktopDepthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_context->GetDevice(), &viewInfo, nullptr, &m_desktopDepthImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create depth image view");
    }
}

void Renderer::CreateVrDepthResources(FrameContext& frame, VkExtent2D extent)
{
    if (frame.vrDepthImageView)
        vkDestroyImageView(m_context->GetDevice(), frame.vrDepthImageView, nullptr);
    if (frame.vrDepthImage)
        vkDestroyImage(m_context->GetDevice(), frame.vrDepthImage, nullptr);
    m_allocator->Free(frame.vrDepthAllocation);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    vkCreateImage(m_context->GetDevice(), &imageInfo, nullptr, &frame.vrDepthImage);
    frame.vrDepthAllocation =
        m_allocator->AllocateImage(frame.vrDepthImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    frame.vrDepthExtent = extent;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = frame.vrDepthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(m_context->GetDevice(), &viewInfo, nullptr, &frame.vrDepthImageView);
}

void Renderer::CreateSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_context->GetDevice(), &semaphoreInfo, nullptr,
                              &m_frames[i].imageAvailableSem) != VK_SUCCESS ||
            vkCreateSemaphore(m_context->GetDevice(), &semaphoreInfo, nullptr,
                              &m_frames[i].renderFinishedSem) != VK_SUCCESS ||
            vkCreateFence(m_context->GetDevice(), &fenceInfo, nullptr, &m_frames[i].inFlightFence) !=
                VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create sync objects");
        }
    }
}

void Renderer::RenderDesktop(const Scene& scene, const glm::mat4& view, const glm::mat4& proj,
                             const glm::vec3& camPos, Editor& editor)
{
    if (m_pipelineDirty)
        RebuildPipeline();

    vkWaitForFences(m_context->GetDevice(), 1, &m_frames[m_currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = m_swapchain->AcquireNextImage(m_frames[m_currentFrame].imageAvailableSem, imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        m_swapchain->Recreate();
        CreateDesktopDepthResources(m_swapchain->GetExtent());
        return;
    }
    else if (result != VK_SUCCESS)
    {
        return;
    }

    vkResetFences(m_context->GetDevice(), 1, &m_frames[m_currentFrame].inFlightFence);

    VkCommandBuffer cmd = m_frames[m_currentFrame].desktopCmd;
    vkResetCommandBuffer(cmd, 0);

    RecordDesktopCommandBuffer(m_frames[m_currentFrame], m_swapchain->GetImages()[imageIndex].imageView,
                               m_swapchain->GetImages()[imageIndex].image, m_swapchain->GetExtent(), scene,
                               view, proj, camPos, editor);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {m_frames[m_currentFrame].imageAvailableSem};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    VkSemaphore signalSemaphores[] = {m_frames[m_currentFrame].renderFinishedSem};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_context->GetGraphicsQueue(), 1, &submitInfo,
                      m_frames[m_currentFrame].inFlightFence) != VK_SUCCESS)
    {
        return;
    }

    m_swapchain->Present(m_frames[m_currentFrame].renderFinishedSem, imageIndex);
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::RenderVR(const Scene& scene, uint32_t viewIndex, VkImageView colorView, VkImage colorImage,
                        VkImageView depthView, VkImage depthImage, VkExtent2D extent, const glm::mat4& view,
                        const glm::mat4& proj, const glm::vec3& camPos)
{
    if (m_pipelineDirty)
        RebuildPipeline();

    FrameContext& frame = m_frames[m_currentFrame];

    if (frame.vrDepthImage == VK_NULL_HANDLE || frame.vrDepthExtent.width != extent.width ||
        frame.vrDepthExtent.height != extent.height)
    {
        CreateVrDepthResources(frame, extent);
    }

    VkCommandBuffer cmd = frame.vrCmds[viewIndex];
    vkResetCommandBuffer(cmd, 0);
    RecordVrCommandBuffer(frame, cmd, colorView, colorImage, depthView, depthImage, extent, scene, view, proj,
                          camPos);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_context->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
}

void Renderer::RecordDesktopCommandBuffer(FrameContext& frame, VkImageView colorView, VkImage colorImage,
                                          VkExtent2D extent, const Scene& scene, const glm::mat4& view,
                                          const glm::mat4& proj, const glm::vec3& camPos, Editor& editor)
{
    VkCommandBuffer cmd = frame.desktopCmd;
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    TransitionImageLayout(cmd, colorImage, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false);
    TransitionImageLayout(cmd, m_desktopDepthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, true);

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = colorView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {0.05f, 0.05f, 0.05f, 1.0f};

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = m_desktopDepthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil.depth = 1.0f;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, extent};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    VkViewport viewport{0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0, 0}, extent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    VkDescriptorSet bindlessSet = m_context->GetBindlessDescriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &bindlessSet, 0,
                            nullptr);

    for (const auto& entity : scene.GetEntities())
    {
        struct PushConstants
        {
            glm::mat4 model, view, proj;
            glm::vec3 camPos, lightDir, lightColor;
            uint32_t albedoTexIndex;
        } pc{entity.transform.GetMatrix(),
             view,
             proj,
             camPos,
             glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)),
             glm::vec3(1.0f),
             entity.albedoTexture ? entity.albedoTexture->GetBindlessIndex() : 0};

        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pc), &pc);
        if (entity.mesh)
            entity.mesh->Draw(cmd);
    }

    editor.RecordDrawData(cmd);

    vkCmdEndRendering(cmd);

    TransitionImageLayout(cmd, colorImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, false);

    vkEndCommandBuffer(cmd);
}

void Renderer::RecordVrCommandBuffer(FrameContext& frame, VkCommandBuffer cmd, VkImageView colorView,
                                     VkImage colorImage, VkImageView depthView, VkImage depthImage,
                                     VkExtent2D extent, const Scene& scene, const glm::mat4& view,
                                     const glm::mat4& proj, const glm::vec3& camPos)
{

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    TransitionImageLayout(cmd, colorImage, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false);

    if (depthImage != VK_NULL_HANDLE)
    {
        TransitionImageLayout(cmd, depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, true);
    }
    else if (frame.vrDepthImage != VK_NULL_HANDLE)
    {
        TransitionImageLayout(cmd, frame.vrDepthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, true);
        depthView = frame.vrDepthImageView;
        depthImage = frame.vrDepthImage;
    }

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = colorView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {0.05f, 0.05f, 0.05f, 1.0f};

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil.depth = 1.0f;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, extent};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    VkViewport viewport{0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0, 0}, extent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    VkDescriptorSet bindlessSet = m_context->GetBindlessDescriptorSet();
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &bindlessSet, 0,
                            nullptr);

    for (const auto& entity : scene.GetEntities())
    {
        struct PushConstants
        {
            glm::mat4 model, view, proj;
            glm::vec3 camPos, lightDir, lightColor;
            uint32_t albedoTexIndex;
        } pc{entity.transform.GetMatrix(),
             view,
             proj,
             camPos,
             glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)),
             glm::vec3(1.0f),
             entity.albedoTexture ? entity.albedoTexture->GetBindlessIndex() : 0};

        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pc), &pc);
        if (entity.mesh)
            entity.mesh->Draw(cmd);
    }

    vkCmdEndRendering(cmd);
    vkEndCommandBuffer(cmd);
}

} // namespace Mirage
