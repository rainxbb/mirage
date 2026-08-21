#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <iostream>

namespace Mirage
{

Model::Model(std::shared_ptr<VulkanContext> context, std::shared_ptr<MemoryAllocator> allocator,
             std::shared_ptr<BindlessAllocator> bindlessAlloc, const std::string& filePath)
    : m_context(context), m_allocator(allocator), m_bindlessAlloc(bindlessAlloc)
{

    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
                         aiProcess_CalcTangentSpace | aiProcess_GlobalScale;

    const aiScene* scene = importer.ReadFile(filePath, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        throw std::runtime_error("Assimp Error loading model: " + std::string(importer.GetErrorString()));
    }

    std::string directory = std::filesystem::path(filePath).parent_path().string();

    ProcessNode(scene->mRootNode, scene, directory);
}

Model::~Model() {}

void Model::Draw(VkCommandBuffer cmdBuffer) const
{
    for (const auto& mesh : m_meshes)
    {
        mesh->Draw(cmdBuffer);
    }
}

void Model::ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(ProcessMesh(mesh, scene, directory));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, directory);
    }
}

std::shared_ptr<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.pos = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

        if (mesh->mNormals)
        {
            vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
        }
        else
        {
            vertex.normal = {0.0f, 1.0f, 0.0f}; // Fallback
        }

        if (mesh->mTextureCoords[0])
        {
            vertex.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        }
        else
        {
            vertex.uv = {0.0f, 0.0f};
        }
        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    auto newMesh = std::make_shared<Mesh>(m_context, m_allocator, vertices, indices);

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        auto albedoTex = LoadMaterialTexture(material, aiTextureType_DIFFUSE, directory);
        if (albedoTex)
        {
            newMesh->SetTexture(albedoTex);
        }
    }

    return newMesh;
}

std::shared_ptr<Texture> Model::LoadMaterialTexture(aiMaterial* mat, aiTextureType type,
                                                    const std::string& directory)
{
    if (mat->GetTextureCount(type) == 0)
    {
        return nullptr;
    }

    aiString str;
    mat->GetTexture(type, 0, &str);

    std::string texturePath = std::string(str.C_Str());

    if (!std::filesystem::path(texturePath).is_absolute())
    {
        texturePath = directory + "/" + texturePath;
    }

    std::cout << "[Model] Attempting to load texture: " << texturePath << "\n";

    for (const auto& loadedTex : m_textures)
    {
        // use AssetManager with a string hash map, cache by path
    }

    try
    {
        auto tex = std::make_shared<Texture>(m_context, m_allocator, m_bindlessAlloc, texturePath);
        m_textures.push_back(tex);
        return tex;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Warning: Failed to load texture: " << texturePath << " (" << e.what() << ")\n";
        return nullptr;
    }
}

} // namespace Mirage
