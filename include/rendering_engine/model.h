#pragma once

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "mesh.h"
#include "scene.h"

class Model 
{
public:
    std::vector<Mesh> meshes;

    Model(const char* path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    void Draw(Shader& shader);
    void DrawMesh(Shader& shader, unsigned int index);

    /**
     * Convert this model into one SceneObject per mesh and add them to a Scene.
     *
     * @param scene    Target scene
     * @param baseName Name prefix; each mesh gets "<baseName>_<index>"
     * @param shader   Shared shader used for all meshes
     * @param mat      Base material applied to every mesh (can be overridden later)
     *
     * Example:
     *   Model knight("models/knight.obj");
     *   auto shader = std::make_shared<Shader>("shaders/cube.vert", "shaders/cube.frag");
     *   knight.toScene(scene, "knight_white", shader);
     */
    void toScene(Scene& scene,
                 const std::string& baseName,
                 std::shared_ptr<Shader> shader,
                 const Material& mat = Material{});

private:
    std::string directory;
    std::vector<Texture> textures_loaded;
    bool gammaCorrection;

    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
                                              std::string typeName);
};