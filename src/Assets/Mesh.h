#pragma once
#include "../Graphics/MemoryAllocator.h"
#include "../Graphics/VulkanContext.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Mirage
{

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Mesh
{
public:
    Mesh(std::shared_ptr<VulkanContext> context, std::shared_ptr<MemoryAllocator> allocator,
         const std::string& path);
    Mesh(std::shared_ptr<VulkanContext> context, std::shared_ptr<MemoryAllocator> allocator,
         const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh();

    void Draw(VkCommandBuffer cmdBuffer, uint32_t instanceCount = 1) const;
    const std::vector<Vertex>& GetVertices() const { return m_vertices; }

private:
    std::shared_ptr<VulkanContext> m_context;
    std::shared_ptr<MemoryAllocator> m_allocator;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    Allocation m_vertexAllocation;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    Allocation m_indexAllocation;
    uint32_t m_indexCount = 0;

    std::vector<Vertex> m_vertices;
};

} // namespace Mirage
