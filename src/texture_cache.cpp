#include "texture_cache.h"
#include <stb_image.h>
#include <glad/glad.h>
#include "logger.h"

Texture TextureCache::load(const std::string& path, const std::string& typeName, bool gammaCorrect)
{
    auto it = m_cache.find(path);
    if (it != m_cache.end())
        return it->second;

    Texture tex;
    tex.id   = loadFromDisk(path, gammaCorrect);
    tex.type = typeName;
    tex.path = path;

    m_cache[path] = tex;
    return tex;
}

void TextureCache::release(const std::string& path)
{
    auto it = m_cache.find(path);
    if (it != m_cache.end())
    {
        glDeleteTextures(1, &it->second.id);
        m_cache.erase(it);
    }
}

void TextureCache::clear()
{
    for (auto& [path, tex] : m_cache)
        glDeleteTextures(1, &tex.id);
    m_cache.clear();
}

unsigned int TextureCache::loadFromDisk(const std::string& path, bool gammaCorrect)
{
    unsigned int id;
    glGenTextures(1, &id);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum internalFormat, dataFormat;
        if (nrChannels == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrChannels == 3)
        {
            internalFormat = gammaCorrect ? GL_SRGB : GL_RGB;
            dataFormat     = GL_RGB;
        }
        else
        {
            internalFormat = gammaCorrect ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat     = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else
    {
        LOG_ERROR_F("TextureCache: failed to load \"%s\"", path.c_str());
        glBindTexture(GL_TEXTURE_2D, id);
        unsigned char pink[] = { 255, 0, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pink);
    }

    stbi_image_free(data);
    return id;
}
