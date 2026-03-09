#pragma once

#include <vector>
#include <string>
#include "glad/glad.h"
#include <glm/glm.hpp>
#include "shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string  type;
    std::string  path;
};

class Mesh {
public:
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;

    Mesh();
    Mesh(std::vector<Vertex>       vertices,
         std::vector<unsigned int> indices,
         std::vector<Texture>      textures);

    // Move constructor — avoids copying large vertex/index buffers
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Non-copyable (each Mesh owns its VAO/VBO/EBO)
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    ~Mesh();

    void Draw(Shader& shader);

    unsigned int getVAO()        const { return VAO; }
    unsigned int getIndexCount() const { return static_cast<unsigned int>(indices.size()); }

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    void setupMesh();
};