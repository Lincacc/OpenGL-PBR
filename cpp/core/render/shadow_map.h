#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

constexpr int kShadowMapSize = 2048;

struct ShadowMap {
    GLuint fbo = 0;
    GLuint depth = 0;
    int size = 0;

    void destroy() {
        if (depth != 0) {
            glDeleteTextures(1, &depth);
            depth = 0;
        }
        if (fbo != 0) {
            glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
        size = 0;
    }

    void create() {
        if (fbo != 0 && depth != 0 && size == kShadowMapSize)
            return;

        destroy();

        glCreateTextures(GL_TEXTURE_2D, 1, &depth);
        if (depth == 0)
            throw std::runtime_error("ShadowMap::create: depth texture failed");
        glTextureStorage2D(depth, 1, GL_DEPTH_COMPONENT32F, kShadowMapSize, kShadowMapSize);
        glTextureParameteri(depth, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(depth, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(depth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(depth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTextureParameteri(depth, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTextureParameteri(depth, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        const float border[4] = { 1.f, 1.f, 1.f, 1.f };
        glTextureParameterfv(depth, GL_TEXTURE_BORDER_COLOR, border);

        glCreateFramebuffers(1, &fbo);
        if (fbo == 0) {
            destroy();
            throw std::runtime_error("ShadowMap::create: framebuffer failed");
        }
        glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depth, 0);
        glNamedFramebufferDrawBuffer(fbo, GL_NONE);
        glNamedFramebufferReadBuffer(fbo, GL_NONE);
        if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            destroy();
            throw std::runtime_error("ShadowMap::create: framebuffer incomplete");
        }

        size = kShadowMapSize;
    }
};

inline glm::mat4 shadow_fit_ortho(const glm::vec3& dir, const glm::vec3& bmin, const glm::vec3& bmax, float floorY)
{
    const glm::vec3 gmin(-4.f, floorY, -4.f);
    const glm::vec3 gmax(4.f, floorY, 4.f);
    const glm::vec3 mn = glm::min(bmin, gmin);
    const glm::vec3 mx = glm::max(bmax, gmax);
    const glm::vec3 center = (mn + mx) * 0.5f;
    const glm::vec3 L = glm::normalize(dir);
    glm::vec3 up(0.f, 1.f, 0.f);
    if (std::abs(L.y) > 0.9f)
        up = glm::vec3(1.f, 0.f, 0.f);
    const float radius = glm::length(mx - mn) * 0.5f + 0.5f;
    const glm::mat4 V = glm::lookAt(center + L * (radius + 2.f), center, up);

    glm::vec3 lmin(1e9f), lmax(-1e9f);
    for (int i = 0; i < 8; ++i) {
        const glm::vec3 c(
            (i & 1) ? mx.x : mn.x,
            (i & 2) ? mx.y : mn.y,
            (i & 4) ? mx.z : mn.z);
        const glm::vec3 p = glm::vec3(V * glm::vec4(c, 1.f));
        lmin = glm::min(lmin, p);
        lmax = glm::max(lmax, p);
    }
    const glm::vec3 pad = (lmax - lmin) * 0.1f;
    lmin -= pad;
    lmax += pad;
    const float zNear = std::max(0.01f, -lmax.z);
    const float zFar = std::max(zNear + 0.01f, -lmin.z);
    return glm::ortho(lmin.x, lmax.x, lmin.y, lmax.y, zNear, zFar) * V;
}
