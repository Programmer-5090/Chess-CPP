#include <mesh.h>

Mesh::Mesh(std::vector<Vertex>       vertices,
           std::vector<unsigned int> indices,
           std::vector<Texture>      textures)
    : vertices(std::move(vertices))
    , indices (std::move(indices))
    , textures(std::move(textures))
{
    setupMesh();
}

Mesh::Mesh(std::vector<Vertex>       vertices,
    std::vector<unsigned int> indices,
    std::vector<Texture>      textures,
    const std::vector<glm::mat4>& transforms)
    : vertices(std::move(vertices))
    , indices(std::move(indices))
    , textures(std::move(textures))
    , instanceTransforms(transforms)
{
    setupMeshInstanced(transforms);
}

Mesh::Mesh() = default;

Mesh::Mesh(Mesh&& other) noexcept
    : vertices(std::move(other.vertices))
    , indices (std::move(other.indices))
    , textures(std::move(other.textures))
    , VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), instanceVBO(other.instanceVBO)
    , instanceCount(other.instanceCount)
    , instanceTransforms(std::move(other.instanceTransforms))
{
    other.VAO = other.VBO = other.EBO = other.instanceVBO = 0;
    other.instanceCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        // Release existing GL objects
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &instanceVBO);

        vertices = std::move(other.vertices);
        indices  = std::move(other.indices);
        textures = std::move(other.textures);
        VAO = other.VAO; VBO = other.VBO; EBO = other.EBO; instanceVBO = other.instanceVBO;
        instanceCount = other.instanceCount;
        instanceTransforms = std::move(other.instanceTransforms);
        other.VAO = other.VBO = other.EBO = other.instanceVBO = 0;
        other.instanceCount = 0;
    }
    return *this;
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &instanceVBO);
}

void Mesh::setupMesh()
{
    if (vertices.empty() || indices.empty()) return;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, Position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, Normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));

    glBindVertexArray(0);
}

void Mesh::setupMeshInstanced(const std::vector<glm::mat4>& transforms) {
    if (vertices.empty() || indices.empty() || transforms.empty()) return;

    instanceCount = static_cast<unsigned int>(transforms.size());

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(), GL_STATIC_DRAW);

    // Vertex attributes (0-2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, Position)));
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, Normal)));
        
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));

    // Instance matrix data setup (mat4 takes 4 vertex attributes: 3, 4, 5, 6)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(transforms.size() * sizeof(glm::mat4)),
        transforms.data(), GL_STATIC_DRAW);

    // Each row of the mat4 is a vec4, occupying one vertex attribute
    for (int i = 0; i < 4; ++i)
    {
        const int attribIdx = 3 + i;
        glEnableVertexAttribArray(attribIdx);
        glVertexAttribPointer(attribIdx, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
            reinterpret_cast<void*>(i * sizeof(glm::vec4)));
        glVertexAttribDivisor(attribIdx, 1);
    }

    glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader)
{
    unsigned int diffuseNr  = 1;
    unsigned int specularNr = 1;
    for (unsigned int i = 0; i < textures.size(); ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        const std::string& name = textures[i].type;
        std::string number;
        if      (name == "texture_diffuse")  number = std::to_string(diffuseNr++);
        else if (name == "texture_specular") number = std::to_string(specularNr++);

        shader.setInt((name + number).c_str(), static_cast<int>(i));
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::DrawInstances(Shader& shader) {
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    for (unsigned int i = 0; i < textures.size(); ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        const std::string& name = textures[i].type;
        std::string number;
        if (name == "texture_diffuse")  number = std::to_string(diffuseNr++);
        else if (name == "texture_specular") number = std::to_string(specularNr++);

        shader.setInt((name + number).c_str(), static_cast<int>(i));
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(VAO);
    glDrawElementsInstanced(
        GL_TRIANGLES,
        static_cast<GLsizei>(indices.size()),
        GL_UNSIGNED_INT,
        nullptr,
        static_cast<GLsizei>(instanceCount)
    );
    glBindVertexArray(0);
}
