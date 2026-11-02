#pragma once

#include <glad/glad.h>

#include <stdexcept>

struct ColorTarget {
    GLuint fbo = 0;
    GLuint tex = 0;
    GLuint depth = 0;

    int w = 0;
    int h = 0;

    void destroy() {
        if (tex != 0) {
            glDeleteTextures(1, &tex);
            tex = 0;
        }
        if (depth != 0) {
            glDeleteRenderbuffers(1, &depth);
            depth = 0;
        }
        if (fbo != 0) {
            glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
        w = 0;
        h = 0;
    }

    void create(int width, int height) {
        if (!(width > 0 && height > 0)) {
            throw std::runtime_error("ColorTarget::create: width and height must be > 0");
        }
        if (fbo != 0 && tex != 0 && depth != 0 && w == width && h == height)
            return;

        destroy();

        glCreateTextures(GL_TEXTURE_2D, 1, &tex);
        if (tex == 0)
            throw std::runtime_error("ColorTarget::create: glCreateTextures failed");

        glTextureStorage2D(tex, 1, GL_RGBA16F, width, height);
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glCreateRenderbuffers(1, &depth);
        if (depth == 0) {
            destroy();
            throw std::runtime_error("ColorTarget::create: glCreateRenderbuffers failed");
        }
        glNamedRenderbufferStorage(depth, GL_DEPTH24_STENCIL8, width, height);

        glCreateFramebuffers(1, &fbo);
        if (fbo == 0) {
            destroy();
            throw std::runtime_error("ColorTarget::create: glCreateFramebuffers failed");
        }
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
        glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
        if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            destroy();
            throw std::runtime_error("ColorTarget::create: framebuffer incomplete");
        }

        w = width;
        h = height;
    }
};
