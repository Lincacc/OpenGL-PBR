#pragma once

#include <glad/glad.h>

#include <stdexcept>

constexpr int kMsaaSamples = 8;

struct ColorTarget {
    GLuint fbo = 0;
    GLuint tex = 0;
    GLuint texIbl = 0;
    GLuint depth = 0;

    GLuint msaaFbo = 0;
    GLuint msaaColor = 0;
    GLuint msaaIbl = 0;
    GLuint msaaDepth = 0;

    int w = 0;
    int h = 0;
    int samples = 0;

    void destroy() {
        auto del_tex = [](GLuint& t) {
            if (t != 0) {
                glDeleteTextures(1, &t);
                t = 0;
            }
        };
        auto del_rb = [](GLuint& rb) {
            if (rb != 0) {
                glDeleteRenderbuffers(1, &rb);
                rb = 0;
            }
        };
        auto del_fbo = [](GLuint& fb) {
            if (fb != 0) {
                glDeleteFramebuffers(1, &fb);
                fb = 0;
            }
        };

        del_tex(tex);
        del_tex(texIbl);
        del_tex(depth);
        del_fbo(fbo);

        del_rb(msaaColor);
        del_rb(msaaIbl);
        del_rb(msaaDepth);
        del_fbo(msaaFbo);

        w = 0;
        h = 0;
        samples = 0;
    }

    void create(int width, int height) {
        if (!(width > 0 && height > 0)) {
            throw std::runtime_error("ColorTarget::create: width and height must be > 0");
        }
        if (fbo != 0 && msaaFbo != 0 && tex != 0 && texIbl != 0 && depth != 0 &&
            w == width && h == height && samples == kMsaaSamples)
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

        auto make_msaa_rb = [width, height](GLuint& rb, GLenum internal) {
            glCreateRenderbuffers(1, &rb);
            if (rb == 0)
                throw std::runtime_error("ColorTarget::create: glCreateRenderbuffers failed");
            glNamedRenderbufferStorageMultisample(rb, kMsaaSamples, internal, width, height);
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
            throw std::runtime_error("ColorTarget::create: resolve framebuffer incomplete");
        }

        try {
            make_msaa_rb(msaaColor, GL_RGBA16F);
            make_msaa_rb(msaaIbl, GL_RGBA16F);
            make_msaa_rb(msaaDepth, GL_DEPTH24_STENCIL8);
        } catch (...) {
            destroy();
            throw;
        }

        glCreateFramebuffers(1, &msaaFbo);
        if (msaaFbo == 0) {
            destroy();
            throw std::runtime_error("ColorTarget::create: msaa framebuffer failed");
        }
        glNamedFramebufferRenderbuffer(msaaFbo, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColor);
        glNamedFramebufferRenderbuffer(msaaFbo, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, msaaIbl);
        glNamedFramebufferRenderbuffer(msaaFbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaaDepth);
        glNamedFramebufferDrawBuffers(msaaFbo, 2, bufs);
        if (glCheckNamedFramebufferStatus(msaaFbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            destroy();
            throw std::runtime_error("ColorTarget::create: msaa framebuffer incomplete");
        }

        w = width;
        h = height;
        samples = kMsaaSamples;
    }

    void resolve() {
        if (msaaFbo == 0 || fbo == 0)
            return;

        glNamedFramebufferReadBuffer(msaaFbo, GL_COLOR_ATTACHMENT0);
        glNamedFramebufferDrawBuffer(fbo, GL_COLOR_ATTACHMENT0);
        glBlitNamedFramebuffer(msaaFbo, fbo, 0, 0, w, h, 0, 0, w, h,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glNamedFramebufferReadBuffer(msaaFbo, GL_COLOR_ATTACHMENT1);
        glNamedFramebufferDrawBuffer(fbo, GL_COLOR_ATTACHMENT1);
        glBlitNamedFramebuffer(msaaFbo, fbo, 0, 0, w, h, 0, 0, w, h,
            GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glBlitNamedFramebuffer(msaaFbo, fbo, 0, 0, w, h, 0, 0, w, h,
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        const GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glNamedFramebufferDrawBuffers(fbo, 2, bufs);
        glNamedFramebufferDrawBuffers(msaaFbo, 2, bufs);
    }
};
