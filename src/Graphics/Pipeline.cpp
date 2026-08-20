#include "Pipeline.h"

#include <stdexcept>

namespace Mirage
{

PipelineBuilder::PipelineBuilder(std::shared_ptr<VulkanContext> context) : m_context(context)
{
    m_inputAssembly = {};
    m_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_inputAssembly.primitiveRestartEnable = VK_FALSE;

    m_rasterizer = {};
    m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_rasterizer.depthClampEnable = VK_FALSE;
    m_rasterizer.rasterizerDiscardEnable = VK_FALSE;
    m_rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    m_rasterizer.lineWidth = 1.0f;
    m_rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    m_rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_rasterizer.depthBiasEnable = VK_FALSE;

    m_multisampling = {};
    m_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_multisampling.sampleShadingEnable = VK_FALSE;
    m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    m_depthStencil = {};
    m_depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_depthStencil.depthTestEnable = VK_TRUE;
    m_depthStencil.depthWriteEnable = VK_TRUE;
    m_depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    m_depthStencil.depthBoundsTestEnable = VK_FALSE;
    m_depthStencil.stencilTestEnable = VK_FALSE;

    m_colorBlendAttachment = {};
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_FALSE;

    m_colorBlendState = {};
    m_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_colorBlendState.logicOpEnable = VK_FALSE;
    m_colorBlendState.attachmentCount = 1;
    m_colorBlendState.pAttachments = &m_colorBlendAttachment;

    m_viewportState = {};
    m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_viewportState.viewportCount = 1;
    m_viewportState.scissorCount = 1;
}

PipelineBuilder::~PipelineBuilder() {}

PipelineBuilder& PipelineBuilder::SetShaders(VkShaderModule vertShader, VkShaderModule fragShader)
{
    m_shaderStages.clear();

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertShader;
    vertStage.pName = "main";
    m_shaderStages.push_back(vertStage);

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragShader;
    fragStage.pName = "main";
    m_shaderStages.push_back(fragStage);

    return *this;
}

PipelineBuilder&
PipelineBuilder::SetVertexInput(const std::vector<VkVertexInputBindingDescription>& bindings,
                                const std::vector<VkVertexInputAttributeDescription>& attributes)
{
    m_vertexInputBindings = bindings;
    m_vertexInputAttributes = attributes;
    return *this;
}

PipelineBuilder& PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
{
    m_inputAssembly.topology = topology;
    return *this;
}

PipelineBuilder& PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
{
    m_rasterizer.polygonMode = mode;
    return *this;
}

PipelineBuilder& PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    m_rasterizer.cullMode = cullMode;
    m_rasterizer.frontFace = frontFace;
    return *this;
}

PipelineBuilder& PipelineBuilder::SetMultisamplingNone()
{
    m_multisampling.sampleShadingEnable = VK_FALSE;
    m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    return *this;
}

PipelineBuilder& PipelineBuilder::DisableDepthTest()
{
    m_depthStencil.depthTestEnable = VK_FALSE;
    m_depthStencil.depthWriteEnable = VK_FALSE;
    return *this;
}

PipelineBuilder& PipelineBuilder::EnableDepthTest(bool depthWriteEnable, VkCompareOp op)
{
    m_depthStencil.depthTestEnable = VK_TRUE;
    m_depthStencil.depthWriteEnable = depthWriteEnable;
    m_depthStencil.depthCompareOp = op;
    return *this;
}

PipelineBuilder& PipelineBuilder::DisableBlending()
{
    m_colorBlendAttachment.blendEnable = VK_FALSE;
    return *this;
}

PipelineBuilder& PipelineBuilder::EnableBlendingAdditive()
{
    m_colorBlendAttachment.blendEnable = VK_TRUE;
    m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    return *this;
}

PipelineBuilder& PipelineBuilder::EnableBlendingAlpha()
{
    m_colorBlendAttachment.blendEnable = VK_TRUE;
    m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    return *this;
}

PipelineBuilder& PipelineBuilder::SetColorAttachmentFormat(VkFormat format)
{
    m_colorAttachmentFormat = format;
    return *this;
}

PipelineBuilder& PipelineBuilder::SetDepthFormat(VkFormat format)
{
    m_depthAttachmentFormat = format;
    return *this;
}

PipelineBuilder& PipelineBuilder::SetPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset,
                                                       uint32_t size)
{
    VkPushConstantRange range{};
    range.stageFlags = stageFlags;
    range.offset = offset;
    range.size = size;
    m_pushConstantRanges.push_back(range);
    return *this;
}

VkPipeline PipelineBuilder::Build(VkPipelineLayout layout)
{
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = (m_colorAttachmentFormat != VK_FORMAT_UNDEFINED) ? 1 : 0;
    renderingInfo.pColorAttachmentFormats =
        (renderingInfo.colorAttachmentCount > 0) ? &m_colorAttachmentFormat : nullptr;
    renderingInfo.depthAttachmentFormat = m_depthAttachmentFormat;
    renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = m_vertexInputBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInputAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = m_vertexInputAttributes.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
    pipelineInfo.pStages = m_shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_inputAssembly;
    pipelineInfo.pViewportState = &m_viewportState;
    pipelineInfo.pRasterizationState = &m_rasterizer;
    pipelineInfo.pMultisampleState = &m_multisampling;
    pipelineInfo.pDepthStencilState = &m_depthStencil;
    pipelineInfo.pColorBlendState = &m_colorBlendState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    pipelineInfo.pDynamicState = &dynamicState;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(m_context->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                  &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    return pipeline;
}

} // namespace Mirage
