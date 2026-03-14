#ifndef MESH_H
#define MESH_H

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
    
    Mesh(std::vector<Vertex>       vertices,
         std::vector<unsigned int> indices,
         std::vector<Texture>      textures,
         const std::vector<glm::mat4>& transforms);

    // Move constructor — avoids copying large vertex/index buffers
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Non-copyable (each Mesh owns its VAO/VBO/EBO)
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    ~Mesh();

    void Draw(Shader& shader);
    void DrawInstances(Shader& shader);

    unsigned int getVAO()         const { return VAO; }
    unsigned int getIndexCount()  const { return static_cast<unsigned int>(indices.size()); }
    unsigned int getInstanceCount() const { return instanceCount; }
    bool isInstanced() const { return instanceCount > 0; }

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0, instanceVBO = 0;
    unsigned int instanceCount = 0;
    void setupMesh();
    void setupMeshInstanced(const std::vector<glm::mat4>& transforms);
};

#endif