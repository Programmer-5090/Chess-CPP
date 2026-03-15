#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include "logger.h"

class Shader {
public:
    unsigned int ID = 0;

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        ID = compileAndLink(readFile(vertexPath).c_str(),
                            readFile(fragmentPath).c_str(),
                            nullptr);
    }

    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
    {
        ID = compileAndLink(readFile(vertexPath).c_str(),
                            readFile(fragmentPath).c_str(),
                            readFile(geometryPath).c_str());
    }

    ~Shader() { if (ID) glDeleteProgram(ID); }

    // Non-copyable — copying a GL program handle is meaningless
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    // Movable
    Shader(Shader&& other) noexcept : ID(other.ID), m_locCache(std::move(other.m_locCache))
    {
        other.ID = 0;
    }
    Shader& operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            if (ID) glDeleteProgram(ID);
            ID           = other.ID;
            m_locCache   = std::move(other.m_locCache);
            other.ID     = 0;
        }
        return *this;
    }

    void use() { glUseProgram(ID); }

    void setBool (const std::string& name, bool  value) const { glUniform1i (loc(name), (int)value); }
    void setInt  (const std::string& name, int   value) const { glUniform1i (loc(name), value); }
    void setFloat(const std::string& name, float value) const { glUniform1f (loc(name), value); }

    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3f(loc(name), value.x, value.y, value.z);
    }
    void setVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4f(loc(name), value.x, value.y, value.z, value.w);
    }
    void setMat3(const std::string& name, const glm::mat3& value) const {
        glUniformMatrix3fv(loc(name), 1, GL_FALSE, glm::value_ptr(value));
    }
    void setMat4(const std::string& name, const glm::mat4& value) const {
        glUniformMatrix4fv(loc(name), 1, GL_FALSE, glm::value_ptr(value));
    }

private:
    // Uniform location cache
    mutable std::unordered_map<std::string, GLint> m_locCache;

    GLint loc(const std::string& name) const
    {
        auto it = m_locCache.find(name);
        if (it != m_locCache.end()) return it->second;
        GLint l = glGetUniformLocation(ID, name.c_str());
        m_locCache.emplace(name, l);
        return l;
    }

    static std::string readFile(const char* path)
    {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(path);
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        } catch (const std::ifstream::failure&) {
            LOG_ERROR_F("Shader: failed to read file: %s", path);
            return {};
        }
    }

    static unsigned int compileStage(GLenum type, const char* src, const char* label)
    {
        unsigned int id = glCreateShader(type);
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);
        int success = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(id, 512, nullptr, log);
            LOG_ERROR_F("Shader compile failed [%s]: %s", label, log);
        }
        return id;
    }

    static unsigned int compileAndLink(const char* vertSrc, const char* fragSrc, const char* geomSrc)
    {
        unsigned int vert = compileStage(GL_VERTEX_SHADER,   vertSrc, "VERTEX");
        unsigned int frag = compileStage(GL_FRAGMENT_SHADER, fragSrc, "FRAGMENT");
        unsigned int geom = 0;
        if (geomSrc && geomSrc[0] != '\0')
            geom = compileStage(GL_GEOMETRY_SHADER, geomSrc, "GEOMETRY");

        unsigned int prog = glCreateProgram();
        glAttachShader(prog, vert);
        glAttachShader(prog, frag);
        if (geom) glAttachShader(prog, geom);
        glLinkProgram(prog);

        int success = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &success);
        if (!success) {
            char log[512];
            glGetProgramInfoLog(prog, 512, nullptr, log);
            LOG_ERROR_F("Shader link failed: %s", log);
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
        if (geom) glDeleteShader(geom);

        return prog;
    }
};

#endif