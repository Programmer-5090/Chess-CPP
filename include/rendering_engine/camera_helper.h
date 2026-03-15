#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <memory>
#include "shader.h"
#include "camera.h"
#include "mesh.h"
// Geometry primitive for the frustum wire-box
#include "cube.h"

/**
 * CameraHelper
 *
 * Draws the view-frustum wireframe of a Camera as seen from another camera.
 * The 8 frustum corners are reconstructed each frame from the target camera's
 * inverse view-projection matrix and uploaded to a dynamic VBO.
 *
 * Usage:
 *   CameraHelper helper;
 *   helper.init();
 *   // each frame (observing `target` from `observer`):
 *   helper.draw(target, observer, projection, aspect);
 */
class CameraHelper
{
public:
    CameraHelper()  = default;
    ~CameraHelper() { destroyDynamic(); }

    CameraHelper(const CameraHelper&)            = delete;
    CameraHelper& operator=(const CameraHelper&) = delete;

    // Call once after a GL context exists
    void init()
    {
        m_shader = std::make_unique<Shader>("shaders/helper.vert", "shaders/helper.frag");
        buildDynamic();
    }

    /**
     * Draw the frustum of `target` as seen from `observer`.
     *
     * @param target     Camera whose frustum to visualise
     * @param observer   Camera you are currently rendering from
     * @param projection Observer's projection matrix
     * @param aspect     Aspect ratio used to build target's projection
     * @param color      Wire colour (default: bright yellow)
     */
    void draw(const Camera&    target,
              const Camera&    observer,
              const glm::mat4& projection,
              float            aspect,
              glm::vec3        color = glm::vec3(1.0f, 0.95f, 0.0f)) const
    {
        if (!m_shader || !m_vao) return;

        updateCorners(target, aspect);

        m_shader->use();
        m_shader->setMat4("uModel",      glm::mat4(1.0f));
        m_shader->setMat4("uView",       observer.GetViewMatrix());
        m_shader->setMat4("uProjection", projection);
        m_shader->setVec3("uColor",      color);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glBindVertexArray(m_vao);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

private:
    // Dynamic VBO — 8 frustum corners updated every draw
    unsigned int m_vao = 0, m_vbo = 0, m_ebo = 0;
    mutable std::array<glm::vec3, 8> m_corners {};
    std::unique_ptr<Shader> m_shader;

    void buildDynamic()
    {
        // 12 edges of a box, each edge = 2 indices
        static const unsigned int idx[24] = {
            0,1, 1,2, 2,3, 3,0,   // near face
            4,5, 5,6, 6,7, 7,4,   // far  face
            0,4, 1,5, 2,6, 3,7    // connecting edges
        };

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

        glBindVertexArray(0);
    }

    void updateCorners(const Camera& cam, float aspect) const
    {
        glm::mat4 invVP = glm::inverse(cam.GetProjectionMatrix(aspect) * cam.GetViewMatrix());

        // NDC corners: near face (z = -1) then far face (z = +1)
        static const glm::vec4 ndc[8] = {
            {-1,-1,-1,1}, { 1,-1,-1,1}, { 1, 1,-1,1}, {-1, 1,-1,1},
            {-1,-1, 1,1}, { 1,-1, 1,1}, { 1, 1, 1,1}, {-1, 1, 1,1}
        };
        for (int i = 0; i < 8; ++i)
        {
            glm::vec4 w = invVP * ndc[i];
            m_corners[i] = glm::vec3(w) / w.w;
        }

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(glm::vec3), m_corners.data());
        glBindVertexArray(0);
    }

    void destroyDynamic()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
    }
};
