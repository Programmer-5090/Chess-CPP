#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shader.h"
#include "mesh.h"
#include "thread_pool.h"
#include "input.h"
#include "camera.h"
#include "renderer.h"
#include "framebuffer.h"
#include "texture_cache.h"
#include "timer.h"
#include "scene.h"
#include "logger.h"
#include "cube.h"

static void processInputs(Input& input, bool& running, SDL_Window* window,
                          Camera& camera, Renderer& renderer, Framebuffer& framebuffer, bool& mouseCaptured)
{
    input.update();
    if (input.shouldQuit() || input.keyDown("Escape"))
        running = false;

    if (input.keyDown("Tab")) {
        mouseCaptured = !mouseCaptured;
        SDL_SetWindowRelativeMouseMode(window, mouseCaptured);
    }

    for (const SDL_Event& e : input.getEvents()) {
        if (e.type == SDL_EVENT_WINDOW_RESIZED) {
            renderer.onResize(e.window.data1, e.window.data2);
            framebuffer.resize(e.window.data1, e.window.data2);
        }
    }
}

int main()
{
    Logger::init("logs", LogLevel::INFO, false);

    ThreadPool pool;
    LOG_INFO_F("ThreadPool started with %zd threads", pool.getThreadCount());

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR_F("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("Chess CPP", 800, 600,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        LOG_ERROR_F("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit(); return 1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        LOG_ERROR_F("SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        LOG_ERROR("Failed to initialize glad");
        return 1;
    }

    LOG_INFO_F("OpenGL %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    SDL_GL_SetSwapInterval(1);

    Camera   camera(glm::vec3(0.0f, 1.0f, 3.0f));
    Renderer renderer(800, 600, camera.Zoom);
    Framebuffer framebuffer(800, 600);

    renderer.setLightPos({ 2.0f, 3.0f, 2.0f });
    renderer.setLightColor({ 1.0f, 1.0f, 1.0f });

    renderer.setClearColor(1.0f, 1.0f, 1.0f);

    Scene scene;
    {
        SceneObject cubeObj;
        cubeObj.name                 = "cube";
        cubeObj.material.objectColor = { 0.4f, 0.6f, 1.0f };
        cubeObj.mesh   = std::make_shared<Mesh>(Cuboid(1.0f).toMesh());
        cubeObj.shader = std::make_shared<Shader>("shaders/cube.vert", "shaders/cube.frag");
        cubeObj.transform.translate(0.0f, 1.0f, 0.0f);
        scene.add(std::move(cubeObj));
    }

    float angle         = 0.0f;
    bool  running       = true;
    bool  mouseCaptured = true;
    Input input;
    Timer timer;

    SDL_SetWindowRelativeMouseMode(window, true);

    while (running)
    {
        timer.tick();
        float deltaTime = timer.deltaTime();

        processInputs(input, running, window, camera, renderer, framebuffer, mouseCaptured);
        if (mouseCaptured)
            camera.ProcessInput(input, deltaTime);

        // Rotate the cube
        angle += 0.01f;
        if (SceneObject* cube = scene.find("cube"))
            cube->transform.setEuler(angle * 0.4f, angle, 0.0f);

        // --- scene pass (off-screen) ---
        framebuffer.bind();
        renderer.beginFrame();

        renderer.drawScene(scene, camera);
        renderer.drawGrid(camera);
        renderer.drawSelected(scene, camera);

        renderer.endFrame();
        framebuffer.unbind();

        // Resolve MSAA → sampleable texture, then blit to default back-buffer
        framebuffer.resolve();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.getResolveFBO());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, framebuffer.getWidth(), framebuffer.getHeight(),
                          0, 0, framebuffer.getWidth(), framebuffer.getHeight(),
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        SDL_GL_SwapWindow(window);
    }

    TextureCache::get().clear();
    Logger::shutdown();

    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
