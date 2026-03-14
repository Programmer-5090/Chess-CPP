#include "renderer.h"
#include "model.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp> // For glm::toMat4

// Constructor

Renderer::Renderer(int width, int height, float fovDegrees)
    : m_width(width), m_height(height), m_fov(fovDegrees)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    updateProjection();

    m_lights.points.push_back({ "default", {2.0f, 3.0f, 2.0f}, {1.0f, 1.0f, 1.0f} });

    m_lightHelper.init();
    m_cameraHelper.init();
    m_outlineShader = std::make_unique<Shader>("shaders/outline.vert",      "shaders/outline.frag");
    m_gridShader    = std::make_unique<Shader>("shaders/infinite_grid.vert",   "shaders/infinite_grid.frag");
    glGenVertexArrays(1, &m_gridVAO);
}

// Frame control

void Renderer::beginFrame()
{
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::endFrame() {}

void Renderer::onResize(int width, int height)
{
    m_width  = width;
    m_height = height;
    glViewport(0, 0, width, height);
    updateProjection();
}

void Renderer::setClearColor(float r, float g, float b, float a)
{
    m_clearColor = { r, g, b, a };
}

void Renderer::setWireframe(bool enabled)
{
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
}

// Uniform helpers

void Renderer::pushUniforms(Shader& shader,
                             const glm::mat4& model,
                             const glm::vec3& viewPos,
                             const Material&  mat) const
{
    shader.use();
    shader.setMat4("uProjection",   m_projection);
    shader.setMat4("uView",         m_view);
    shader.setMat4("uModel",        model);
    shader.setMat3("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(model))));

    // Light uniforms
    glm::vec3 lightPos   { 2.0f, 3.0f, 2.0f };
    glm::vec3 lightColor { 1.0f, 1.0f, 1.0f };
    float     lightConst { 1.0f  };
    float     lightLin   { 0.09f };
    float     lightQuad  { 0.032f};

    if (!m_lights.points.empty())
    {
        const auto& pl = m_lights.points[0];
        lightPos   = pl.position;
        lightColor = pl.color * pl.intensity;
        lightConst = pl.constant;
        lightLin   = pl.linear;
        lightQuad  = pl.quadratic;
    }
    else if (!m_lights.directionals.empty())
    {
        const auto& dl = m_lights.directionals[0];
        lightPos   = -dl.direction * 100.0f;
        lightColor =  dl.color * dl.intensity;
    }

    shader.setVec3 ("uLightPos",        lightPos);
    shader.setVec3 ("uLightColor",      lightColor);
    shader.setFloat("uLightConstant",   lightConst);
    shader.setFloat("uLightLinear",     lightLin);
    shader.setFloat("uLightQuadratic",  lightQuad);
    shader.setVec3 ("uViewPos",         viewPos);

    // Ambient uniforms (from LightList)
    shader.setVec3 ("uAmbientColor",    m_lights.ambientColor);
    shader.setFloat("uAmbientStrength", m_lights.ambientStrength);

    // Per-object material
    shader.setVec3 ("uObjectColor",    mat.objectColor);
    shader.setVec3 ("uSpecularColor",  mat.specularColor);
    shader.setFloat("uShininess",      mat.shininess);
    shader.setBool ("uUseTexture",     mat.useTexture);
    shader.setBool ("uUseSpecularMap", mat.useSpecularMap);

    if (mat.useTexture && mat.diffuseTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture);
        shader.setInt("texture_diffuse1", 0);
    }
    if (mat.useSpecularMap && mat.specularTexture)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mat.specularTexture);
        shader.setInt("texture_specular1", 1);
    }
}

void Renderer::applyUniforms(Shader& shader, const Camera& camera, const glm::mat4& model) const
{
    m_view       = camera.GetViewMatrix();
    m_projection = camera.GetProjectionMatrix(
        static_cast<float>(m_width) / static_cast<float>(m_height));
    pushUniforms(shader, model, camera.Position, Material{});
}

// Internal helpers

// Draw all meshes of one object — handles both single-mesh and multi-mesh paths
static void drawObject(const SceneObject& obj, Shader& shader)
{
    if (obj.model)
        for (auto& m : obj.model->meshes) const_cast<Mesh&>(m).Draw(shader);
    else if (obj.mesh)
    {
        if (obj.mesh->isInstanced())
            const_cast<Mesh&>(*obj.mesh).DrawInstances(shader);
        else
            obj.mesh->Draw(shader);
    }
}

// Scene draw (per-object shaders)

void Renderer::drawScene(Scene& scene, const Camera& camera) const
{
    m_view       = camera.GetViewMatrix();
    m_projection = camera.GetProjectionMatrix(
        static_cast<float>(m_width) / static_cast<float>(m_height));

    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    for (auto& obj : scene.objects())
    {
        if (!obj.visible || !obj.shader) continue;
        if (!obj.mesh && !obj.model)     continue;

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        pushUniforms(*obj.shader, obj.transform.matrix(), camera.Position, obj.material);
        drawObject(obj, *obj.shader);
    }

    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
}

// Scene draw (forced shader — shadow / depth / picking pass)

void Renderer::drawScene(Scene& scene, const Camera& camera, Shader& overrideShader) const
{
    m_view       = camera.GetViewMatrix();
    m_projection = camera.GetProjectionMatrix(
        static_cast<float>(m_width) / static_cast<float>(m_height));

    overrideShader.use();
    overrideShader.setMat4("uProjection", m_projection);
    overrideShader.setMat4("uView",       m_view);

    for (auto& obj : scene.objects())
    {
        if (!obj.visible) continue;
        if (!obj.mesh && !obj.model) continue;

        overrideShader.setMat4("uModel", obj.transform.matrix());
        drawObject(obj, overrideShader);
    }
}

// Selection outline (stencil two-pass) 

void Renderer::drawSelected(Scene& scene, const Camera& camera,
                             glm::vec3 outlineColor, float outlineScale) const
{
    if (!m_outlineShader) return;

    m_view = camera.GetViewMatrix();

    // Early-exit: check if anything visible+selected has geometry (mesh OR model)
    bool anySelected = false;
    for (const auto& obj : scene.objects())
        if (obj.visible && obj.selected && (obj.mesh || obj.model))
            { anySelected = true; break; }
    if (!anySelected) return;

    // Pass 1: write 1 into stencil for every selected fragment
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    for (auto& obj : scene.objects())
    {
        if (!obj.visible || !obj.selected || !obj.shader) continue;
        if (!obj.mesh && !obj.model) continue;

        pushUniforms(*obj.shader, obj.transform.matrix(), camera.Position, obj.material);
        drawObject(obj, *obj.shader);
    }

    // Pass 2: scaled-up mesh where stencil != 1 - visible border 
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);

    m_outlineShader->use();
    m_outlineShader->setMat4("uProjection",  m_projection);
    m_outlineShader->setMat4("uView",        m_view);
    m_outlineShader->setVec3("uOutlineColor", outlineColor);

    for (auto& obj : scene.objects())
    {
        if (!obj.visible || !obj.selected) continue;
        if (!obj.mesh && !obj.model) continue;

        // Scale around the object's local origin, then re-apply TRS
        glm::mat4 scaled = glm::translate(glm::mat4(1.0f), obj.transform.position)
                         * glm::toMat4(obj.transform.rotation)
                         * glm::scale(glm::mat4(1.0f), obj.transform.scale * outlineScale);
        m_outlineShader->setMat4("uModel", scaled);
        drawObject(obj, *m_outlineShader);
    }

    // Restore GL state 
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glEnable(GL_DEPTH_TEST);
}

// Infinite LOD grid

void Renderer::drawGrid(const Camera& camera,
                        float gridSize,
                        float cellSize,
                        float minPixelsBetweenCells,
                        float alpha) const
{
    if (!m_gridShader) return;

    glm::mat4 vp = m_projection * camera.GetViewMatrix();

    m_gridShader->use();
    m_gridShader->setMat4 ("gVP",                       vp);
    m_gridShader->setVec3 ("gCameraWorldPos",            camera.Position);
    m_gridShader->setFloat("gGridSize",                  gridSize);
    m_gridShader->setFloat("gGridCellSize",              cellSize);
    m_gridShader->setFloat("gGridMinPixelsBetweenCells", minPixelsBetweenCells);
    m_gridShader->setVec4 ("gGridColorThin",             glm::vec4(0.5f));
    m_gridShader->setVec4 ("gGridColorThick",            glm::vec4(1.0f));
    m_gridShader->setFloat("gGridAlpha",                 alpha);

    // Read depth so the grid is occluded by opaque geometry, but don't write
    // depth — the grid is a transparent overlay and must not clobber the scene
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Restore state
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

// Helper draw calls

void Renderer::drawLightHelpers(const Camera& camera) const
{
    m_lightHelper.draw(m_lights, camera, m_projection);
}

void Renderer::drawCameraHelper(const Camera& target, const Camera& observer) const
{
    float aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
    m_cameraHelper.draw(target, observer, m_projection, aspect);
}

// Projection

void Renderer::updateProjection(const Camera* cam)
{
    const float aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
    m_projection = cam
        ? cam->GetProjectionMatrix(aspect)
        : glm::perspective(glm::radians(m_fov), aspect, 0.1f, 1000.0f);
}
