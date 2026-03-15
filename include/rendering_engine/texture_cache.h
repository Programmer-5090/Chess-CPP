#pragma once

#include <string>
#include <unordered_map>
#include "mesh.h"

class TextureCache
{
public:
    static TextureCache& get()
    {
        static TextureCache instance;
        return instance;
    }

    Texture load(const std::string& path, const std::string& typeName, bool gammaCorrect = false);
    void release(const std::string& path);
    void clear();

private:
    TextureCache() = default;
    ~TextureCache() = default;  // GL context may be gone; call clear() explicitly before shutdown
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    std::unordered_map<std::string, Texture> m_cache;

    unsigned int loadFromDisk(const std::string& path, bool gammaCorrect);
};
