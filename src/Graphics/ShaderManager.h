#pragma once
#include "../Core/FileWatcher.h"
#include "VulkanContext.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace Mirage
{

class ShaderManager
{
public:
    ShaderManager(std::shared_ptr<VulkanContext> context, const std::string& shaderDir);
    ~ShaderManager();

    VkShaderModule GetShader(const std::string& filename);
    void ReloadShader(const std::string& filepath);
    void SetReloadCallback(std::function<void()> callback) { m_reloadCallback = callback; }

private:
    VkShaderModule CreateShaderModule(const std::vector<char>& buffer);
    std::vector<char> ReadFile(const std::string& filename);

    std::shared_ptr<VulkanContext> m_context;
    std::string m_shaderDir;
    FileWatcher m_watcher;
    std::function<void()> m_reloadCallback;

    struct ShaderData
    {
        VkShaderModule module = VK_NULL_HANDLE;
        std::string path;
    };
    std::unordered_map<std::string, ShaderData> m_shaders;
};

} // namespace Mirage
