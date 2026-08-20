#include "ShaderManager.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace Mirage
{

ShaderManager::ShaderManager(std::shared_ptr<VulkanContext> context, const std::string& shaderDir)
    : m_context(context), m_shaderDir(shaderDir)
{

    m_watcher.WatchDirectory(shaderDir,
                             [this](const std::string& path)
                             {
                                 std::cout << "[HotReload] Shader modified: " << path << "\n";
                                 ReloadShader(path);
                             });
}

ShaderManager::~ShaderManager()
{
    m_watcher.Stop();
    for (auto& [name, data] : m_shaders)
    {
        if (data.module)
            vkDestroyShaderModule(m_context->GetDevice(), data.module, nullptr);
    }
}

VkShaderModule ShaderManager::GetShader(const std::string& filename)
{
    if (m_shaders.find(filename) == m_shaders.end())
    {
        std::string fullPath = m_shaderDir + "/" + filename;
        std::vector<char> buffer = ReadFile(fullPath);
        VkShaderModule module = CreateShaderModule(buffer);

        m_shaders[filename] = {module, fullPath};
    }
    return m_shaders[filename].module;
}

void ShaderManager::ReloadShader(const std::string& filepath)
{
    for (auto& [name, data] : m_shaders)
    {
        if (data.path == filepath)
        {
            std::vector<char> buffer = ReadFile(filepath);
            VkShaderModule newModule = CreateShaderModule(buffer);

            if (data.module)
                vkDestroyShaderModule(m_context->GetDevice(), data.module, nullptr);
            data.module = newModule;

            if (m_reloadCallback)
                m_reloadCallback();
            break;
        }
    }
}

VkShaderModule ShaderManager::CreateShaderModule(const std::vector<char>& buffer)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_context->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }
    return shaderModule;
}

std::vector<char> ShaderManager::ReadFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

} // namespace Mirage
