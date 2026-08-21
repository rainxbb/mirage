#pragma once
#include "../Graphics/BindlessAllocator.h"
#include "../Graphics/MemoryAllocator.h"
#include "../Graphics/VulkanContext.h"
#include "Mesh.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <memory>
#include <string>
#include <vector>

namespace Mirage
{

class Model
{
public:
    Model(std::shared_ptr<VulkanContext> context, std::shared_ptr<MemoryAllocator> allocator,
          std::shared_ptr<BindlessAllocator> bindlessAlloc, const std::string& filePath);
    ~Model();

    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_meshes; }

    void Draw(VkCommandBuffer cmdBuffer) const;

private:
    void ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory);
    std::shared_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory);
    std::shared_ptr<Texture> LoadMaterialTexture(aiMaterial* mat, const std::string& directory);

    std::shared_ptr<VulkanContext> m_context;
    std::shared_ptr<MemoryAllocator> m_allocator;
    std::shared_ptr<BindlessAllocator> m_bindlessAlloc;

    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::vector<std::shared_ptr<Texture>> m_textures;
};

} // namespace Mirage
