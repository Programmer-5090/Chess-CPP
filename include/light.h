#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

// Point light
struct PointLight {
    std::string name;
    glm::vec3   position  { 0.0f, 2.0f, 0.0f };
    glm::vec3   color     { 1.0f, 1.0f, 1.0f };
    float       intensity { 1.0f };

    // Attenuation coefficients (Ogre/learnopengl convention)
    float constant  { 1.0f  };
    float linear    { 0.09f };
    float quadratic { 0.032f };

    bool visible { true };  // whether the helper sphere is drawn
};

// Directional light
struct DirectionalLight {
    std::string name;
    glm::vec3   direction { -0.2f, -1.0f, -0.3f };
    glm::vec3   color     { 1.0f,  1.0f,  1.0f  };
    float       intensity { 1.0f };

    bool visible { true };
};

// Scene-wide light list
struct LightList {
    std::vector<PointLight>       points;
    std::vector<DirectionalLight> directionals;

    // Ambient applied to every fragment regardless of light count
    glm::vec3 ambientColor    { 1.0f };
    float     ambientStrength { 0.15f };

    // Helpers
    PointLight*       findPoint      (const std::string& name);
    DirectionalLight* findDirectional(const std::string& name);
};

inline PointLight* LightList::findPoint(const std::string& name)
{
    for (auto& l : points)
        if (l.name == name) return &l;
    return nullptr;
}

inline DirectionalLight* LightList::findDirectional(const std::string& name)
{
    for (auto& l : directionals)
        if (l.name == name) return &l;
    return nullptr;
}
