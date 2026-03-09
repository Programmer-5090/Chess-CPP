#include <iostream>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"

// ---- shader sources --------------------------------------------------------

static const char* vertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 uMVP;

out vec3 vColor;
void main()
{
    vColor      = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* fragSrc = R"(
#version 330 core
in  vec3 vColor;
out vec4 FragColor;
void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";

// ---- cube geometry ---------------------------------------------------------

// 8 unique vertices (pos + color)
static const float vertices[] = {
    // position            color
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,
};

static const unsigned int indices[] = {
    0,1,2, 2,3,0, // back
    4,5,6, 6,7,4, // front
    0,4,7, 7,3,0, // left
    1,5,6, 6,2,1, // right
    3,2,6, 6,7,3, // top
    0,1,5, 5,4,0, // bottom
};

// ---- shader helpers --------------------------------------------------------

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error: " << log << "\n";
    }
    return s;
}

static GLuint buildProgram(const char* vs, const char* fs)
{
    GLuint vert = compileShader(GL_VERTEX_SHADER,   vs);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert); glAttachShader(prog, frag);
    glLinkProgram(prog);
    int ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Link error: " << log << "\n";
    }
    glDeleteShader(vert); glDeleteShader(frag);
    return prog;
}

// ---- main ------------------------------------------------------------------

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("Cube Test", 800, 600,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize glad\n";
        return 1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";

    SDL_GL_SetSwapInterval(1);
    glEnable(GL_DEPTH_TEST);

    // GPU objects
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    GLuint program = buildProgram(vertSrc, fragSrc);
    GLint  mvpLoc  = glGetUniformLocation(program, "uMVP");

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.5f));

    float angle  = 0.0f;
    bool  running = true;
    SDL_Event e;

    while (running)
    {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
        }

        angle += 0.01f;
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle,        glm::vec3(0.0f, 1.0f, 0.0f));
        model            = glm::rotate(model,           angle * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 mvp   = proj * view * model;

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        SDL_GL_SwapWindow(window);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(program);
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
