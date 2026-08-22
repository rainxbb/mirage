#pragma once
#include "../Assets/Mesh.h"
#include "../Assets/Texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Mirage
{

struct Transform
{
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 GetMatrix() const
    {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 R = glm::mat4_cast(rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return T * R * S;
    }
};

struct Material
{
    glm::vec4 albedoTint{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float _pad1 = 0.0f;
};

struct Entity
{
    std::string name;
    Transform transform;
    Material material;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Texture> albedoTexture;
};

class Scene
{
public:
    void AddEntity(const Entity& entity) { m_entities.push_back(entity); }

    const std::vector<Entity>& GetEntities() const { return m_entities; }
    std::vector<Entity>& GetEntities() { return m_entities; }

private:
    std::vector<Entity> m_entities;
};

} // namespace Mirage
