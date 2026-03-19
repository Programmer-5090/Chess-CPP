#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.h"
#include "camera.h"
#include "scene.h"
#include "light.h"
#include "light_helper.h"
#include "camera_helper.h"

class Renderer
{
public:
    Renderer(int width, int height, float fovDegrees = 45.0f);
    ~Renderer();

    void beginFrame();
    void endFrame();
    void onResize(int width, int height);
    void setClearColor(float r, float g, float b, float a = 1.0f);
    void setWireframe(bool enabled);

    const glm::mat4& getProjection() const { return m_projection; }
    const glm::mat4& getView()       const { return m_view; }

    // Light management
    LightList&       lights()       { return m_lights; }
    const LightList& lights() const { return m_lights; }

    // Convenience setters kept for backward compatibility
    void setLightPos  (const glm::vec3& pos)   { if (!m_lights.points.empty()) m_lights.points[0].position = pos; }
    void setLightColor(const glm::vec3& color) { if (!m_lights.points.empty()) m_lights.points[0].color    = color; }

    // Draw calls
    // Apply view/proj/light + material uniforms for one manually-drawn object
    void applyUniforms(Shader& shader, const Camera& camera, const glm::mat4& model) const;

    // Draw every visible SceneObject with each object's own shader + material
    void drawScene(Scene& scene, const Camera& camera) const;

    // Draw every visible SceneObject with a single forced shader (shadow/pick pass)
    // Material uniforms are NOT pushed — caller sets what it needs before the call
    void drawScene(Scene& scene, const Camera& camera, Shader& overrideShader) const;

    // Draw all visible 2D sprites from Scene in screen space (top-left origin)
    void drawScene2D(Scene& scene) const;

    // Draw stencil outlines for every selected SceneObject (call after drawScene)
    void drawSelected(Scene& scene, const Camera& camera,
                      glm::vec3 outlineColor = glm::vec3(1.0f, 0.75f, 0.0f),
                      float     outlineScale = 1.04f) const;

    // Draw the infinite LOD grid (call after drawScene, before drawSelected)
    void drawGrid(const Camera& camera,
                  float gridSize             = 100.0f,
                  float cellSize             = 0.025f,
                  float minPixelsBetweenCells = 2.0f,
                  float alpha                = 0.5f) const;

    // Draw wireframe helpers for all visible lights
    void drawLightHelpers(const Camera& camera) const;

    // Draw the frustum wireframe of `target` as seen from `observer`
    void drawCameraHelper(const Camera& target, const Camera& observer) const;

private:
    int   m_width, m_height;
    float m_fov;
    mutable glm::mat4 m_projection;
    mutable glm::mat4 m_view { 1.0f };
    glm::vec4 m_clearColor { 1.0f, 1.0f, 1.0f, 1.0f };

    LightList    m_lights;
    LightHelper  m_lightHelper;
    CameraHelper m_cameraHelper;

    // Flat-colour unlit shader used for the stencil outline pass
    std::unique_ptr<Shader> m_outlineShader;

    // Infinite LOD grid shader + its VAO (no VBO needed — VS uses gl_VertexID)
    std::unique_ptr<Shader> m_gridShader;
    GLuint                  m_gridVAO = 0;

    // 2D sprite pipeline
    std::unique_ptr<Shader> m_spriteShader;
    GLuint                  m_spriteVAO = 0;
    GLuint                  m_spriteVBO = 0;
    GLuint                  m_spriteEBO = 0;

    void updateProjection(const Camera* cam = nullptr);

    // Upload the full per-object uniform set into a shader
    void pushUniforms(Shader& shader,
                      const glm::mat4& model,
                      const glm::vec3& viewPos,
                      const Material&  mat) const;
};
