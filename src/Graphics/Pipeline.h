#pragma once
#include "VulkanContext.h"

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Mirage
{

class PipelineBuilder
{
public:
    PipelineBuilder(std::shared_ptr<VulkanContext> context);
    ~PipelineBuilder();

    PipelineBuilder& SetShaders(VkShaderModule vertShader, VkShaderModule fragShader);
    PipelineBuilder& SetVertexInput(const std::vector<VkVertexInputBindingDescription>& bindings,
                                    const std::vector<VkVertexInputAttributeDescription>& attributes);
    PipelineBuilder& SetInputTopology(VkPrimitiveTopology topology);
    PipelineBuilder& SetPolygonMode(VkPolygonMode mode);
    PipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    PipelineBuilder& SetMultisamplingNone();
    PipelineBuilder& DisableDepthTest();
    PipelineBuilder& EnableDepthTest(bool depthWriteEnable, VkCompareOp op);
    PipelineBuilder& DisableBlending();
    PipelineBuilder& EnableBlendingAdditive();
    PipelineBuilder& EnableBlendingAlpha();
    PipelineBuilder& SetColorAttachmentFormat(VkFormat format);
    PipelineBuilder& SetDepthFormat(VkFormat format);
    PipelineBuilder& SetPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);

    VkPipeline Build(VkPipelineLayout layout);

private:
    std::shared_ptr<VulkanContext> m_context;

    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

    std::vector<VkVertexInputBindingDescription> m_vertexInputBindings;
    std::vector<VkVertexInputAttributeDescription> m_vertexInputAttributes;

    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
    VkPipelineRasterizationStateCreateInfo m_rasterizer;
    VkPipelineMultisampleStateCreateInfo m_multisampling;
    VkPipelineDepthStencilStateCreateInfo m_depthStencil;
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo m_colorBlendState;
    VkPipelineViewportStateCreateInfo m_viewportState;

    VkFormat m_colorAttachmentFormat = VK_FORMAT_UNDEFINED;
    VkFormat m_depthAttachmentFormat = VK_FORMAT_UNDEFINED;

    std::vector<VkPushConstantRange> m_pushConstantRanges;
};

} // namespace Mirage
