#pragma once

#include <glad/glad.h>

class Framebuffer
{
public:
    // samples > 1 enables MSAA (e.g. 4); pass 1 for no antialiasing
    explicit Framebuffer(int width, int height, int samples = 4);
    ~Framebuffer();

    // Non-copyable, movable
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void bind()   const;
    void unbind() const;
    void resize(int width, int height);

    // Blit MSAA FBO ? resolve FBO so the resolved texture can be sampled
    // No-op when samples == 1 (color texture is already sampleable)
    void resolve() const;

    // FBO used for rendering (MSAA when samples > 1)
    unsigned int getFBO()          const { return m_fbo; }

    // Resolve FBO — plain GL_TEXTURE_2D result after resolve() (MSAA only)
    unsigned int getResolveFBO()   const { return m_samples > 1 ? m_resolveFBO : m_fbo; }

    // Sampleable GL_TEXTURE_2D color result (after resolve() for MSAA)
    unsigned int getColorTexture() const { return m_samples > 1 ? m_resolveTex : m_colorTexture; }

    // Raw MSAA texture (only valid when samples > 1)
    unsigned int getMSAATexture()  const { return m_colorTexture; }

    int  getWidth()   const { return m_width; }
    int  getHeight()  const { return m_height; }
    int  getSamples() const { return m_samples; }
    bool isComplete() const;

private:
    // Main (MSAA) FBO
    unsigned int m_fbo          = 0;
    unsigned int m_colorTexture = 0;  // GL_TEXTURE_2D_MULTISAMPLE when samples > 1
    unsigned int m_rbo          = 0;

    // Resolve FBO (only allocated when samples > 1)
    unsigned int m_resolveFBO = 0;
    unsigned int m_resolveTex = 0;

    int m_width, m_height;
    int m_samples;

    void create();
    void destroy();
};
