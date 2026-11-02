#pragma once

#include "core/shader_program.h"
#include "core/render/full_quad.h"
#include "core/shaders/shader_code_ssao.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>

#include <array>
#include <cmath>
#include <random>
#include <stdexcept>

constexpr int kSsaoKernelSize = 64;

struct Ssao {
    bool enabled = true;
    float radius = 0.5f;
    float bias = 0.025f;
    float strength = 1.f;

    Shader shader;
    Shader blurShader;
    GLuint raw = 0;
    GLuint blurred = 0;
    GLuint rawFbo = 0;
    GLuint blurFbo = 0;
    GLuint noise = 0;
    glm::vec3 samples[kSsaoKernelSize]{};
    int w = 0;
    int h = 0;
    bool ready = false;

    void destroy_targets() {
        if (raw != 0) {
            glDeleteTextures(1, &raw);
            raw = 0;
        }
        if (blurred != 0) {
            glDeleteTextures(1, &blurred);
            blurred = 0;
        }
        if (rawFbo != 0) {
            glDeleteFramebuffers(1, &rawFbo);
            rawFbo = 0;
        }
        if (blurFbo != 0) {
            glDeleteFramebuffers(1, &blurFbo);
            blurFbo = 0;
        }
        w = 0;
        h = 0;
    }

    void destroy() {
        destroy_targets();
        if (noise != 0) {
            glDeleteTextures(1, &noise);
            noise = 0;
        }
        ready = false;
    }

    void init() {
        if (ready)
            return;

        shader.compile(vSsaoShaderCode, fSsaoShaderCode);
        blurShader.compile(vSsaoShaderCode, fSsaoBlurShaderCode);

        std::mt19937 rng(8);
        std::uniform_real_distribution<float> unit(0.f, 1.f);
        std::uniform_real_distribution<float> signed_unit(-1.f, 1.f);
        for (int i = 0; i < kSsaoKernelSize; ++i) {
            glm::vec3 s(signed_unit(rng), signed_unit(rng), unit(rng));
            s = glm::normalize(s) * unit(rng);
            const float t = float(i) / float(kSsaoKernelSize);
            const float scale = 0.1f + t * t * 0.9f;
            samples[i] = s * scale;
        }

        std::array<glm::vec3, 16> noise_tex{};
        for (auto& n : noise_tex)
            n = glm::vec3(signed_unit(rng), signed_unit(rng), 0.f);

        glCreateTextures(GL_TEXTURE_2D, 1, &noise);
        if (noise == 0)
            throw std::runtime_error("Ssao::init noise texture failed");
        glTextureStorage2D(noise, 1, GL_RGB16F, 4, 4);
        glTextureSubImage2D(noise, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, noise_tex.data());
        glTextureParameteri(noise, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(noise, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(noise, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(noise, GL_TEXTURE_WRAP_T, GL_REPEAT);

        ready = true;
    }

    void create(int width, int height) {
        init();
        if (!(width > 0 && height > 0))
            throw std::runtime_error("Ssao::create: width and height must be > 0");
        if (raw != 0 && blurred != 0 && w == width && h == height)
            return;

        destroy_targets();

        auto make_ao = [width, height](GLuint& tex, GLuint& fbo) {
            glCreateTextures(GL_TEXTURE_2D, 1, &tex);
            if (tex == 0)
                throw std::runtime_error("Ssao::create texture failed");
            glTextureStorage2D(tex, 1, GL_R16F, width, height);
            glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glCreateFramebuffers(1, &fbo);
            if (fbo == 0)
                throw std::runtime_error("Ssao::create framebuffer failed");
            glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
            if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("Ssao::create framebuffer incomplete");

            const float one[4] = { 1.f, 1.f, 1.f, 1.f };
            glClearNamedFramebufferfv(fbo, GL_COLOR, 0, one);
        };

        try {
            make_ao(raw, rawFbo);
            make_ao(blurred, blurFbo);
        } catch (...) {
            destroy_targets();
            throw;
        }

        w = width;
        h = height;
    }

    void draw(GLuint depthTex, const glm::mat4& projection, const FullscreenQuad& quad) {
        if (!enabled || depthTex == 0 || w <= 0 || h <= 0)
            return;

        const glm::mat4 inv_proj = glm::inverse(projection);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, rawFbo);
        shader.bind();
        shader.setInt("depthMap", 0);
        shader.setInt("noiseMap", 1);
        shader.setVec3Array("samples", samples, kSsaoKernelSize);
        shader.setMatrix4("projectionMatrix", projection);
        shader.setMatrix4("invProjection", inv_proj);
        shader.setVec2f("noiseScale", glm::vec2(float(w) / 4.f, float(h) / 4.f));
        shader.setFloat("radius", radius);
        shader.setFloat("bias", bias);
        shader.setInt("kernelSize", kSsaoKernelSize);
        glBindTextureUnit(0, depthTex);
        glBindTextureUnit(1, noise);
        quad.draw();

        glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);
        blurShader.bind();
        blurShader.setInt("ssaoMap", 0);
        blurShader.setInt("depthMap", 1);
        blurShader.setMatrix4("invProjection", inv_proj);
        blurShader.setVec2f("texelSize", glm::vec2(1.f / float(w), 1.f / float(h)));
        glBindTextureUnit(0, raw);
        glBindTextureUnit(1, depthTex);
        quad.draw();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};
