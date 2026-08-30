#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stdexcept>

namespace Mirage
{

Mesh::Mesh(std::shared_ptr<VulkanContext> context, std::shared_ptr<MemoryAllocator> allocator,
           const std::string& path)
    : m_context(context), m_allocator(allocator)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                       aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        throw std::runtime_error("Assimp Error: " + std::string(importer.GetErrorString()));
    }

    aiMesh* mesh = scene->mMeshes[0];
    m_vertices.reserve(mesh->mNumVertices);
    std::vector<uint32_t> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.pos = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
        vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
        if (mesh->mTextureCoords[0])
        {
            vertex.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        }
        else
        {
            vertex.uv = {0.0f, 0.0f};
        }
        m_vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    m_indexCount = static_cast<uint32_t>(indices.size());

    VkDeviceSize vertexSize = sizeof(Vertex) * m_vertices.size();
    VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

    VkBuffer vertexStaging, indexStaging;
    Allocation vertexStagingAlloc, indexStagingAlloc;

    m_allocator->CreateBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              vertexStaging, vertexStagingAlloc);
    memcpy(vertexStagingAlloc.mappedData, m_vertices.data(), vertexSize);

    m_allocator->CreateBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              indexStaging, indexStagingAlloc);
    memcpy(indexStagingAlloc.mappedData, indices.data(), indexSize);

    m_allocator->CreateBuffer(vertexSize,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexAllocation);
    m_allocator->CreateBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexAllocation);

    VkCommandBuffer cmd = m_context->BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = vertexSize;
    vkCmdCopyBuffer(cmd, vertexStaging, m_vertexBuffer, 1, &copyRegion);

    copyRegion.size = indexSize;
    vkCmdCopyBuffer(cmd, indexStaging, m_indexBuffer, 1, &copyRegion);

    m_context->EndSingleTimeCommands(cmd);

    vkDestroyBuffer(m_context->GetDevice(), vertexStaging, nullptr);
    vkDestroyBuffer(m_context->GetDevice(), indexStaging, nullptr);
    m_allocator->Free(vertexStagingAlloc);
    m_allocator->Free(indexStagingAlloc);
}

Mesh::Mesh(std::shared_ptr<VulkanContext> context, std::shared_ptr<MemoryAllocator> allocator,
           const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : m_context(context), m_allocator(allocator), m_vertices(vertices)
{
    boundMin = glm::vec3(1e9f);
    boundMax = glm::vec3(-1e9f);
    for (const auto& v : vertices)
    {
        boundMin = glm::min(boundMin, v.pos);
        boundMax = glm::max(boundMax, v.pos);
    }

    m_indexCount = static_cast<uint32_t>(indices.size());
    VkDeviceSize vertexSize = sizeof(Vertex) * vertices.size();
    VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

    VkBuffer vertexStaging, indexStaging;
    Allocation vertexStagingAlloc, indexStagingAlloc;

    m_allocator->CreateBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              vertexStaging, vertexStagingAlloc);
    memcpy(vertexStagingAlloc.mappedData, vertices.data(), vertexSize);

    m_allocator->CreateBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              indexStaging, indexStagingAlloc);
    memcpy(indexStagingAlloc.mappedData, indices.data(), indexSize);

    m_allocator->CreateBuffer(vertexSize,
                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexAllocation);
    m_allocator->CreateBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexAllocation);

    VkCommandBuffer cmd = m_context->BeginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = vertexSize;
    vkCmdCopyBuffer(cmd, vertexStaging, m_vertexBuffer, 1, &copyRegion);
    copyRegion.size = indexSize;
    vkCmdCopyBuffer(cmd, indexStaging, m_indexBuffer, 1, &copyRegion);
    m_context->EndSingleTimeCommands(cmd);

    vkDestroyBuffer(m_context->GetDevice(), vertexStaging, nullptr);
    vkDestroyBuffer(m_context->GetDevice(), indexStaging, nullptr);
    m_allocator->Free(vertexStagingAlloc);
    m_allocator->Free(indexStagingAlloc);
}

Mesh::~Mesh()
{
    if (m_vertexBuffer)
        vkDestroyBuffer(m_context->GetDevice(), m_vertexBuffer, nullptr);
    if (m_indexBuffer)
        vkDestroyBuffer(m_context->GetDevice(), m_indexBuffer, nullptr);
    m_allocator->Free(m_vertexAllocation);
    m_allocator->Free(m_indexAllocation);
}

void Mesh::Draw(VkCommandBuffer cmdBuffer, uint32_t instanceCount) const
{
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuffer, m_indexCount, instanceCount, 0, 0, 0);
}

} // namespace Mirage
