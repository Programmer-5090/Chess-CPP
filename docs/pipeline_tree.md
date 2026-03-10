# Chess CPP — Rendering Pipeline Tree

This document describes the full frame-render pipeline and the compile-time module
dependency tree for the Chess CPP project.

---

## 1. Frame Render Pipeline

Every frame produced by `main.cpp` passes through the following ordered stages:

### Stage 1: Timer Update
- **Timer::tick()** — Compute deltaTime (SDL_GetTicks delta / 1000)

### Stage 2: Input Processing
- **Input::update()** — SDL_PollEvent loop
  - Track keyboard down/up/held states
  - Update mouse position + delta
  - Handle `SDL_EVENT_WINDOW_RESIZED` ? call `Renderer::onResize(w, h)` ? `Framebuffer::resize(w, h)`
  - Detect Escape key ? set `running = false`
  - Detect Tab key ? toggle SDL relative mouse mode

### Stage 3: Camera Input (when mouse captured)
- **Camera::ProcessInput()**
  - W/S/A/D/E/Q ? `Camera::ProcessKeyboard()`
  - Mouse delta ? `Camera::ProcessMouseMovement()`

### Stage 4: Game Logic
- Update scene state (e.g., rotate cube via `SceneObject::transform.setEuler()`)

### Stage 5: Off-Screen Render Pass

#### 5a. Framebuffer Setup
- `Framebuffer::bind()` — Bind MSAA FBO, set viewport

#### 5b. Frame Clear
- `Renderer::beginFrame()` — `glClear(COLOR | DEPTH | STENCIL)`

#### 5c. Scene Rendering
- `Renderer::drawScene(scene, camera)`
  - Compute `camera.GetViewMatrix()` ? `m_view`
  - Compute `camera.GetProjectionMatrix()` ? `m_projection`
  - Set `glStencilOp(KEEP, KEEP, REPLACE)`
  - For each visible SceneObject with shader + geometry:
    - `glStencilFunc(ALWAYS, 1, 0xFF)`
    - `Renderer::pushUniforms()` — Bind:
      - `uProjection, uView, uModel, uNormalMatrix`
      - `uLightPos/Color/Constant/Linear/Quadratic`
      - `uAmbientColor / uAmbientStrength`
      - `uObjectColor / uSpecularColor / uShininess`
      - `GL_TEXTURE_2D` for diffuse + specular
    - `drawObject()` — Choose path:
      - *Model path*: For each Mesh, call `Mesh::Draw(shader)` ? `glDrawElements(GL_TRIANGLES, ...)`
      - *Single Mesh path*: `Mesh::Draw(shader)` ? `glDrawElements(GL_TRIANGLES, ...)`

#### 5d. Grid Rendering
- `Renderer::drawGrid(camera)`
  - Bind `m_gridShader`, set uniforms: `gVP, gCameraWorldPos, gGridSize, gGridCellSize, gGridMinPixelsBetweenCells, gGridAlpha`
  - `glDepthMask(GL_FALSE) + glEnable(GL_BLEND)`
  - `glBindVertexArray(m_gridVAO)` ? `glDrawArrays(GL_TRIANGLES, 0, 6)` (vertex shader uses `gl_VertexID`)

#### 5e. Selection Outline Rendering (Stencil-based)
- `Renderer::drawSelected(scene, camera)`
  - **Pass 1** — Write stencil buffer for selected objects:
    - Use same `pushUniforms + drawObject` as drawScene
    - Mark pixels with `stencil = 1`
  - **Pass 2** — Draw outline around selected objects:
    - `glStencilFunc(NOTEQUAL, 1, 0xFF)` — Only render where stencil ? 1
    - `glDisable(GL_DEPTH_TEST)`
    - Bind `m_outlineShader`, set uniforms: `uProjection, uView, uModel, uOutlineColor`
    - Call `drawObject()` with scaled transform

#### 5f. End Frame
- `Renderer::endFrame()` — (Currently a no-op placeholder)

### Stage 6: MSAA Resolve
- `Framebuffer::unbind()`
- `Framebuffer::resolve()`
  - `glBlitFramebuffer(MSAA FBO ? resolve FBO)` — Downsample MSAA texture
  - `glBlitFramebuffer(resolve FBO ? default back-buffer)` — Copy to screen

### Stage 7: Present
- `SDL_GL_SwapWindow()` — Display frame on screen

---

## 2. Shader Pipeline

### 2a. Geometry / Lighting Pass  (`cube.vert` ? `cube.frag`)

```
CPU (Renderer::pushUniforms)
  ?  uModel, uView, uProjection, uNormalMatrix
  ?  uLightPos, uLightColor, uLightConstant/Linear/Quadratic
  ?  uAmbientColor, uAmbientStrength
  ?  uObjectColor, uSpecularColor, uShininess
  ?  uUseTexture, uUseSpecularMap
  ?  texture_diffuse1 (unit 0), texture_specular1 (unit 1)
  ?
cube.vert
  ?  vFragPos  = uModel * aPos
  ?  vNormal   = uNormalMatrix * aNormal
  ?  vTexCoords = aTexCoords
  ?  gl_Position = uProjection * uView * vec4(vFragPos, 1.0)
  ?
cube.frag
  ?? attenuation = 1 / (C + L·d + Q·d²)
  ?? ambient     = uAmbientStrength * uAmbientColor * uLightColor
  ?? diffuse     = max(dot(N, L), 0) * uLightColor * attenuation
  ?? specular    = pow(dot(N, H), shininess) * specSample * uLightColor * attenuation
  ?? FragColor   = (ambient + diffuse + specular) * baseColor
```

### 2b. Stencil Outline Pass  (`outline.vert` ? `outline.frag`)

```
CPU (Renderer::drawSelected – Pass 2)
  ?  uModel (scaled), uView, uProjection, uOutlineColor
  ?
outline.vert  ?  gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0)
  ?
outline.frag  ?  FragColor = vec4(uOutlineColor, 1.0)
```

### 2c. Infinite LOD Grid  (`infinite_grid.vert` ? `infinite_grid.frag`)

```
CPU (Renderer::drawGrid)
  ?  gVP, gCameraWorldPos, gGridSize, gGridCellSize,
  ?  gGridMinPixelsBetweenCells, gGridAlpha
  ?
infinite_grid.vert
  ?  Reads 4-vertex quad from constant array (no VBO)
  ?  Scales by gGridSize, offsets by camera XZ position
  ?  Outputs WorldPos
  ?
infinite_grid.frag
  ?? Computes screen-space derivative (dFdx / dFdy) of WorldPos
  ?? Calculates LOD level from pixel-per-cell density
  ?? Blends thin/thick grid lines across LOD0/LOD1/LOD2
  ?? OpacityFalloff = fade at grid edge
  ?? FragColor with alpha < 0.01 discarded
```

### 2d. Debug Helper Pass  (`helper.vert` ? `helper.frag`)

```
CPU (LightHelper::draw / CameraHelper::draw)
  ?  uModel, uView, uProjection, uColor
  ?
helper.vert  ?  gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0)
  ?
helper.frag  ?  FragColor = vec4(uColor, 1.0)
```

---

## 3. Module / Class Dependency Tree

```
main.cpp
??? SDL3                         (window, GL context, events)
??? glad                         (OpenGL function loader)
??? glm                          (math)
?
??? Logger         [utils/]
?   ??? (static singleton, thread-safe, file + console output)
?
??? ThreadPool     [utils/]
?   ??? TaskQueue
?   ??? Thread
?
??? Timer          [include/]
?   ??? SDL_GetTicks
?
??? Input          [include/]
?   ??? SDL_PollEvent
?
??? Camera         [include/ + src/]
?   ??? GetViewMatrix()          glm::lookAt
?   ??? GetProjectionMatrix()    glm::perspective / glm::ortho
?   ??? ProcessKeyboard()
?   ??? ProcessMouseMovement()
?   ??? ProcessMouseScroll()
?   ??? ProcessInput()           ? Input
?
??? Shader         [include/]    (header-only)
?   ??? reads .vert / .frag / .geom source files
?   ??? glCompileShader / glLinkProgram
?   ??? uniform location cache (unordered_map)
?
??? ComputeShader  [include/]    (header-only, static utility class)
?   ??? LoadComputeShader()
?   ??? CreateBuffer()
?   ??? BindBuffer()
?   ??? Dispatch()
?   ??? ReadBuffer<T>()
?   ??? WriteBuffer<T>()
?
??? Mesh           [include/ + src/]
?   ??? Vertex  { Position, Normal, TexCoords }
?   ??? Texture { id, type, path }
?   ??? setupMesh()   ? VAO/VBO/EBO
?   ??? Draw()        ? glDrawElements
?
??? Model          [include/ + src/]
?   ??? Assimp::Importer
?   ??? loadModel()   ? processNode() ? processMesh()
?   ??? loadMaterialTextures() ? TextureCache::get().load()
?   ??? Draw()
?   ??? DrawMesh()
?   ??? toScene()     ? Scene
?
??? TextureCache   [include/ + src/]   (singleton)
?   ??? load()        ? loadFromDisk()  (stb_image)
?   ??? release()
?   ??? clear()
?
??? Transform      [include/]    (header-only struct)
?   ??? position, scale, rotation (quaternion)
?   ??? translate(), setEuler(), rotate(), slerpTo(), lookAt()
?   ??? matrix()  ? T * R * S
?
??? Material       [include/scene.h]   (plain struct)
?   ??? objectColor, specularColor, shininess
?   ??? useTexture, useSpecularMap
?   ??? diffuseTexture, specularTexture (GL texture IDs)
?
??? SceneObject    [include/scene.h]   (plain struct)
?   ??? name, transform, material
?   ??? mesh  (single-mesh path)
?   ??? shader
?   ??? model (multi-mesh path)
?   ??? visible
?   ??? selected
?
??? Scene          [include/ + src/]
?   ??? add()
?   ??? find()
?   ??? remove()
?   ??? objects()
?
??? Light          [include/]    (header-only structs)
?   ??? PointLight       { name, position, color, intensity, attenuation, visible }
?   ??? DirectionalLight { name, direction, color, intensity, visible }
?   ??? LightList        { points[], directionals[], ambientColor, ambientStrength }
?       ??? findPoint()
?       ??? findDirectional()
?
??? Framebuffer    [include/ + src/]
?   ??? MSAA FBO  (GL_TEXTURE_2D_MULTISAMPLE + RBO)
?   ??? Resolve FBO (plain GL_TEXTURE_2D)
?   ??? bind() / unbind()
?   ??? resolve()   glBlitFramebuffer
?   ??? resize()    destroy() + create()
?
??? Renderer       [include/ + src/]
?   ??? beginFrame()
?   ??? endFrame()
?   ??? onResize()
?   ??? setClearColor() / setWireframe()
?   ??? drawScene()         ? Scene, Camera
?   ??? drawScene(override) ? Scene, Camera, Shader
?   ??? drawSelected()      ? Scene, Camera  (stencil outline)
?   ??? drawGrid()          ? Camera
?   ??? drawLightHelpers()  ? LightHelper
?   ??? drawCameraHelper()  ? CameraHelper
?   ??? applyUniforms()
?   ??? pushUniforms()      ? Shader, Material
?   ??? updateProjection()
?
??? LightHelper    [include/]    (header-only)
?   ??? init()   ? Shader + Sphere.toMesh() + Cuboid.toMesh()
?   ??? draw()   ? wireframe spheres + arrows per light
?
??? CameraHelper   [include/]    (header-only)
?   ??? init()           ? dynamic VAO/VBO/EBO
?   ??? draw()           ? updateCorners() + glDrawElements
?   ??? updateCorners()  ? inverse VP * NDC corners ? world space
?
??? Geometry       [geometry/]
    ?
    ??? GeometryData    [geometry_data.h / .cpp]
    ?   ??? addAttribute()
    ?   ??? setIndices()
    ?   ??? countVertices() / countIndices()
    ?   ??? getTriangles()
    ?   ??? toMesh()          ? Mesh
    ?   ??? merge()
    ?   ??? clear()
    ?
    ??? Cuboid      [cube.h]     (header-only, extends GeometryData)
    ?   ??? constructor:  6-face, 24-vertex box with normals + UVs
    ?
    ??? Polygon     [polygon.h / polygon.cpp]  (extends GeometryData)
    ?   ??? constructor:  flat N-sided regular polygon (fan triangulation)
    ?
    ??? ParametricSurface  [parametric.h / parametric.cpp]  (extends GeometryData)
    ?   ??? generateMesh()   ? user-supplied surface_fn(u, v) ? glm::vec3
    ?
    ??? Sphere      [sphere.h / sphere.cpp]  (extends ParametricSurface)
    ?   ??? constructor:  UV sphere via sphereFunc(u, v)
    ?
    ??? Circle      [circle.h]   (extends Polygon)
        ??? constructor:  Polygon(segments, radius)  (alias for a circular polygon)
```

---

## 4. Data Flow Summary

```
Disk assets                GL context              CPU structures
???????????                ??????????              ??????????????
.vert / .frag  ??????????? Shader (GL program)
model files    ?Assimp???  Mesh (VAO/VBO/EBO)  ??? SceneObject
textures       ?stb??????? GL_TEXTURE_2D        ??? Material
                                                     ?
                                         Transform ???
                                                     ?
                                         Scene ???????
                                           ?
                                           ?
                                      Renderer
                                           ?
                              ???????????????????????????
                              ?            ?             ?
                         drawScene    drawGrid      drawSelected
                              ?
                         pushUniforms ? Shader uniforms
                              ?
                         Mesh::Draw ? glDrawElements
                              ?
                         Framebuffer (MSAA)
                              ? resolve()
                         Framebuffer (resolve) ? blit ? back-buffer
                              ?
                         SDL_GL_SwapWindow ? screen
```
