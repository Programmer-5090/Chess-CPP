#pragma once

#include <string>
#include <vector>
#include <memory>
#include "mesh.h"
#include "shader.h"
#include "transform.h"
#include "glm/vec3.hpp"

// Forward declaration — avoids pulling all of model.h into every TU that includes scene.h
class Model;

// Per-object material 
struct Material {
    glm::vec3 objectColor   { 1.0f, 1.0f, 1.0f };  // diffuse colour when no texture
    glm::vec3 specularColor { 0.5f, 0.5f, 0.5f };  // specular tint when no specular map
    float     shininess     { 32.0f };

    bool useTexture    { false };
    bool useSpecularMap{ false };

    // Optional texture overrides (texture IDs managed by TextureCache)
    unsigned int diffuseTexture  { 0 };
    unsigned int specularTexture { 0 };
};

// Scene object 
struct SceneObject {
    std::string name;
    Transform   transform;
    Material    material;

    // Single-mesh path (procedural / GeometryData)
    std::shared_ptr<Mesh>   mesh;
    std::shared_ptr<Shader> shader;

    // Multi-mesh path (Assimp Model). When set, drawScene iterates model->meshes
    // instead of the single mesh field.
    std::shared_ptr<Model>  model;

    bool visible  { true };   // set false for captured pieces without removing them
    bool selected { false };  // drives stencil outline highlight
};

// Scene 
class Scene {
public:
    SceneObject* add(SceneObject obj);
    SceneObject* find(const std::string& name);
    void remove(const std::string& name);

    const std::vector<SceneObject>& objects() const { return m_objects; }
          std::vector<SceneObject>& objects()       { return m_objects; }

private:
    std::vector<SceneObject> m_objects;
};
