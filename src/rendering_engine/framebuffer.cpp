#include "framebuffer.h"
#include "logger.h"

Framebuffer::Framebuffer(int width, int height, int samples)
    : m_width(width), m_height(height), m_samples(samples)
{
    create();
}

Framebuffer::~Framebuffer()
{
    destroy();
}

void Framebuffer::create()
{
    // Main FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    if (m_samples > 1)
    {
        glGenTextures(1, &m_colorTexture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_colorTexture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_RGB,
                                m_width, m_height, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D_MULTISAMPLE, m_colorTexture, 0);

        glGenRenderbuffers(1, &m_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples,
                                         GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_rbo);
    }
    else
    {
        glGenTextures(1, &m_colorTexture);
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_colorTexture, 0);

        glGenRenderbuffers(1, &m_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_rbo);
    }

    if (!isComplete())
        LOG_ERROR("Framebuffer: main FBO incomplete after creation");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Resolve FBO (MSAA only)
    if (m_samples > 1)
    {
        glGenFramebuffers(1, &m_resolveFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFBO);

        glGenTextures(1, &m_resolveTex);
        glBindTexture(GL_TEXTURE_2D, m_resolveTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_resolveTex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            LOG_ERROR("Framebuffer: resolve FBO incomplete after creation");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Framebuffer::destroy()
{
    glDeleteTextures(1, &m_colorTexture);
    glDeleteRenderbuffers(1, &m_rbo);
    glDeleteFramebuffers(1, &m_fbo);

    if (m_samples > 1)
    {
        glDeleteTextures(1, &m_resolveTex);
        glDeleteFramebuffers(1, &m_resolveFBO);
    }

    m_colorTexture = m_rbo = m_fbo = 0;
    m_resolveTex   = m_resolveFBO  = 0;
}

void Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resolve() const
{
    if (m_samples <= 1) return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFBO);
    glBlitFramebuffer(0, 0, m_width, m_height,
                      0, 0, m_width, m_height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int width, int height)
{
    m_width  = width;
    m_height = height;
    destroy();
    create();
}

bool Framebuffer::isComplete() const
{
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}
