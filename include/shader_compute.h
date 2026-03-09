#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include "logger.h"

class ComputeShader {
public:
    static GLuint LoadComputeShader(const std::string& filePath) {
        std::string computeSource = ReadFile(filePath);
        if (computeSource.empty()) {
            LOG_ERROR_F("ComputeShader: failed to read file: %s", filePath.c_str());
            return 0;
        }

        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
        const char* source = computeSource.c_str();
        glShaderSource(computeShader, 1, &source, nullptr);
        glCompileShader(computeShader);

        GLint success = 0;
        glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[1024];
            glGetShaderInfoLog(computeShader, 1024, nullptr, infoLog);
            LOG_ERROR_F("ComputeShader compile failed (%s): %s", filePath.c_str(), infoLog);
            glDeleteShader(computeShader);
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, computeShader);
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[1024];
            glGetProgramInfoLog(program, 1024, nullptr, infoLog);
            LOG_ERROR_F("ComputeShader link failed (%s): %s", filePath.c_str(), infoLog);
            glDeleteProgram(program);
            glDeleteShader(computeShader);
            return 0;
        }

        glDeleteShader(computeShader);
        return program;
    }

    static GLuint CreateBuffer(size_t size, const void* data = nullptr, GLenum usage = GL_DYNAMIC_DRAW) {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return buffer;
    }

    static void BindBuffer(GLuint buffer, GLuint binding) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer);
    }

    static void Dispatch(GLuint program, GLuint numGroupsX, GLuint numGroupsY = 1, GLuint numGroupsZ = 1) {
        glUseProgram(program);
        glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    static GLuint GetThreadGroupSizes(int numThreads, int groupSize = 64) {
        return static_cast<GLuint>((numThreads + groupSize - 1) / groupSize);
    }

    template<typename T>
    static std::vector<T> ReadBuffer(GLuint buffer, size_t count) {
        std::vector<T> data(count);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
        void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        if (ptr) {
            std::memcpy(data.data(), ptr, count * sizeof(T));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return data;
    }

    template<typename T>
    static void WriteBuffer(GLuint buffer, const std::vector<T>& data) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
        GLsizeiptr sizeBytes = static_cast<GLsizeiptr>(data.size() * sizeof(T));
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeBytes, nullptr, GL_DYNAMIC_DRAW);
        void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeBytes,
                                     GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        if (ptr) {
            std::memcpy(ptr, data.data(), sizeBytes);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        } else {
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeBytes, data.data());
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    static void Release(GLuint& buffer) {
        if (buffer != 0) { glDeleteBuffers(1, &buffer); buffer = 0; }
    }

    static void ReleaseProgram(GLuint& program) {
        if (program != 0) { glDeleteProgram(program); program = 0; }
    }

private:
    static std::string ReadFile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) return {};
        std::stringstream buf;
        buf << file.rdbuf();
        return buf.str();
    }
};
