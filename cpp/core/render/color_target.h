#pragma once

#include <glad/glad.h>

#include <stdexcept>

struct ColorTarget {
    GLuint fbo = 0;
    GLuint tex = 0;
    GLuint texIbl = 0;
    GLuint depth = 0;

    int w = 0;
    int h = 0;

    void destroy() {
        if (tex != 0) {
            glDeleteTextures(1, &tex);
            tex = 0;
        }
        if (texIbl != 0) {
            glDeleteTextures(1, &texIbl);
            texIbl = 0;
        }
        if (depth != 0) {
            glDeleteTextures(1, &depth);
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
        if (fbo != 0 && tex != 0 && texIbl != 0 && depth != 0 && w == width && h == height)
            return;

        destroy();

        auto make_color = [width, height](GLuint& out) {
            glCreateTextures(GL_TEXTURE_2D, 1, &out);
            if (out == 0)
                throw std::runtime_error("ColorTarget::create: glCreateTextures failed");
            glTextureStorage2D(out, 1, GL_RGBA16F, width, height);
            glTextureParameteri(out, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(out, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(out, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(out, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        };

        try {
            make_color(tex);
            make_color(texIbl);
        } catch (...) {
            destroy();
            throw;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &depth);
        if (depth == 0) {
            destroy();
            throw std::runtime_error("ColorTarget::create: depth texture failed");
        }
        glTextureStorage2D(depth, 1, GL_DEPTH24_STENCIL8, width, height);
        glTextureParameteri(depth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(depth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(depth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(depth, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        glCreateFramebuffers(1, &fbo);
        if (fbo == 0) {
            destroy();
            throw std::runtime_error("ColorTarget::create: glCreateFramebuffers failed");
        }
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT1, texIbl, 0);
        glNamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, depth, 0);
        const GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glNamedFramebufferDrawBuffers(fbo, 2, bufs);
        if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            destroy();
            throw std::runtime_error("ColorTarget::create: framebuffer incomplete");
        }

        w = width;
        h = height;
    }
};
