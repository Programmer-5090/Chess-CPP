#include <model.h>
#include "texture_cache.h"
#include "logger.h"

void Model::loadModel(std::string path)
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        LOG_ERROR_F("Model: Assimp error loading \"%s\": %s", path.c_str(), import.GetErrorString());
        return;
    }
    // Support both POSIX and Windows path separators when extracting directory
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos)
        directory = path.substr(0, pos);
    else
        directory = ".";

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(processMesh(mesh, scene));			
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
} 

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        glm::vec3 vector; 
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z; 
        vertex.Position = vector;

        // Normals may be missing for some meshes; guard against null pointer
        if (mesh->HasNormals() && mesh->mNormals)
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        else
        {
            vertex.Normal = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        // process material

        // Texture coordinates: check for the presence of channel 0
        if (mesh->HasTextureCoords(0) && mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }
    // process indices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    
    if(mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<Texture> diffuseMaps  = loadMaterialTextures(material, aiTextureType_DIFFUSE,  "texture_diffuse");
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), diffuseMaps.begin(),  diffuseMaps.end());
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }  

    return Mesh(std::move(vertices), std::move(indices), std::move(textures));
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string fullPath = directory + '/' + str.C_Str();
        textures.push_back(TextureCache::get().load(fullPath, typeName, gammaCorrection));
    }
    return textures;
}  

void Model::Draw(Shader &shader)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader);
}

void Model::DrawMesh(Shader &shader, unsigned int index)
{
    if (index < meshes.size())
        meshes[index].Draw(shader);
}

void Model::toScene(Scene& scene,
                    const std::string& baseName,
                    std::shared_ptr<Shader> shader,
                    const Material& mat)
{
    for (unsigned int i = 0; i < meshes.size(); ++i)
    {
        SceneObject obj;
        obj.name     = baseName + (meshes.size() > 1 ? "_" + std::to_string(i) : "");
        obj.shader   = shader;
        obj.material = mat;
        obj.mesh     = std::make_shared<Mesh>(std::move(meshes[i]));

        // If the mesh has its own textures, let the material reflect that
        if (!meshes[i].textures.empty())
        {
            obj.material.useTexture = true;
            for (const auto& tex : meshes[i].textures)
            {
                if (tex.type == "texture_diffuse" && obj.material.diffuseTexture == 0)
                    obj.material.diffuseTexture = tex.id;
                else if (tex.type == "texture_specular" && obj.material.specularTexture == 0)
                {
                    obj.material.specularTexture = tex.id;
                    obj.material.useSpecularMap  = true;
                }
            }
        }

        scene.add(std::move(obj));
    }
}
