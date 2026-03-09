#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include "shader.h"
#include "light.h"
#include "camera.h"
#include "mesh.h"
// Geometry primitives
#include "sphere.h"
#include "cube.h"

/**
 * LightHelper
 *
 * Draws a small wireframe sphere at every PointLight position and a small
 * wireframe cuboid arrow-head for every DirectionalLight.
 * Both meshes are built once from the project's GeometryData system.
 *
 * Usage:
 *   LightHelper helper;
 *   helper.init();                              // once, after GL context
 *   helper.draw(lights, camera, projection);    // each frame
 */
class LightHelper
{
public:
    LightHelper()  = default;
    ~LightHelper() = default;

    LightHelper(const LightHelper&)            = delete;
    LightHelper& operator=(const LightHelper&) = delete;

    // Build GL objects — call once after the GL context exists
    void init()
    {
        m_shader = std::make_unique<Shader>("shaders/helper.vert", "shaders/helper.frag");

        // Point-light indicator: low-poly sphere (radius 1, 6 segments ? 12 tris)
        Sphere sphereGeo(1.0f, 6);
        m_sphereMesh = std::make_unique<Mesh>(sphereGeo.toMesh());

        // Directional-light indicator: flat thin cuboid (acts as arrow head)
        Cuboid arrowGeo(0.08f, 1.0f, 0.08f);
        m_arrowMesh = std::make_unique<Mesh>(arrowGeo.toMesh());
    }

    // Draw all visible light helpers using observer camera + its projection matrix
    void draw(const LightList& lights,
              const Camera&    camera,
              const glm::mat4& projection) const
    {
        if (!m_shader || !m_sphereMesh || !m_arrowMesh) return;

        // Draw in wireframe only for helpers
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        m_shader->use();
        m_shader->setMat4("uView",       camera.GetViewMatrix());
        m_shader->setMat4("uProjection", projection);

        // ?? Point lights: small sphere ??????????????????????????????????
        for (const auto& light : lights.points)
        {
            if (!light.visible) continue;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), light.position);
            model           = glm::scale(model, glm::vec3(0.15f));
            m_shader->setMat4("uModel", model);
            m_shader->setVec3("uColor", light.color);
            m_sphereMesh->Draw(*m_shader);
        }

        // ?? Directional lights: thin cuboid rotated along direction ?????
        for (const auto& light : lights.directionals)
        {
            if (!light.visible) continue;

            glm::vec3 dir   = glm::normalize(light.direction);
            glm::mat4 model(1.0f);
            model           = glm::scale(model, glm::vec3(1.5f * light.intensity));

            // Rotate default up-axis (0,1,0) onto the light direction
            const glm::vec3 up(0.0f, 1.0f, 0.0f);
            glm::vec3 axis = glm::cross(up, dir);
            float sinA = glm::length(axis);
            float cosA = glm::dot(up, dir);
            if (sinA > 1e-5f)
                model = glm::rotate(model, glm::atan(sinA, cosA), glm::normalize(axis));

            m_shader->setMat4("uModel", model);
            m_shader->setVec3("uColor", light.color);
            m_arrowMesh->Draw(*m_shader);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

private:
    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<Mesh>   m_sphereMesh;  // point-light indicator
    std::unique_ptr<Mesh>   m_arrowMesh;   // directional-light indicator
};
