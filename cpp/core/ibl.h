#pragma once

#include "shader_program.h"
#include "shaders/shader_code_ibl.h"
#include "shaders/shader_code_skybox.h"
#include "skybox.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int kIblPrefilterSize = 256;
constexpr int kIblPrefilterMips = 9;
constexpr float kIblPrefilterMaxLod = 8.f;
constexpr int kIblPrefilterSamples = 256;
constexpr int kIblLutSize = 128;
constexpr int kIblLutSamples = 1024;

inline void ibl_texture_destroy(GLuint& tex) {
    if (tex != 0) {
        glDeleteTextures(1, &tex);
        tex = 0;
    }
}

inline const glm::mat4& ibl_cube_proj() {
    static const glm::mat4 proj = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 10.f);
    return proj;
}

inline const glm::mat4* ibl_cube_views() {
    static const glm::mat4 views[6] = {
        glm::lookAt(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, -1.f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, -1.f, 0.f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, -1.f, 0.f)),
    };
    return views;
}

inline bool ibl_bind_cube_vao() {
    static GLuint vao = 0, vbo = 0;
    if (vao == 0) {
        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        if (vao == 0 || vbo == 0) {
            if (vao != 0)
                glDeleteVertexArrays(1, &vao);
            if (vbo != 0)
                glDeleteBuffers(1, &vbo);
            vao = 0;
            vbo = 0;
            return false;
        }
        glNamedBufferStorage(vbo, sizeof(kUnitCubeTriangles), kUnitCubeTriangles, 0);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 3);
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }
    glBindVertexArray(vao);
    return true;
}

inline GLuint ibl_brdf_lut_create() {
    static Shader lut_shader;
    if (lut_shader.programHandle == 0)
        lut_shader.compile(vIblLutShaderCode, fIblLutShaderCode);

    GLuint lut = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &lut);
    if (lut == 0)
        return 0;

    glTextureStorage2D(lut, 1, GL_RG16F, kIblLutSize, kIblLutSize);
    glTextureParameteri(lut, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(lut, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(lut, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(lut, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint fbo = 0;
    glCreateFramebuffers(1, &fbo);
    if (fbo == 0) {
        glDeleteTextures(1, &lut);
        return 0;
    }

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);
    if (vao == 0) {
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &lut);
        return 0;
    }

    GLint vp[4] = {};
    glGetIntegerv(GL_VIEWPORT, vp);
    GLint prev_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lut, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &lut);
        return 0;
    }

    glViewport(0, 0, kIblLutSize, kIblLutSize);
    glDisable(GL_DEPTH_TEST);
    lut_shader.bind();
    lut_shader.setInt("sampleCount", kIblLutSamples);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glEnable(GL_DEPTH_TEST);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    return lut;
}

inline GLuint ibl_prefilter_create(GLuint env_cube) {
    if (env_cube == 0)
        return 0;

    static Shader prefilter;
    if (prefilter.programHandle == 0)
        prefilter.compile(vEquirectToCubeShaderCode, fIblPrefilterShaderCode);

    GLint src_size = 0;
    glGetTextureLevelParameteriv(env_cube, 0, GL_TEXTURE_WIDTH, &src_size);
    if (src_size <= 0)
        return 0;

    GLuint cube = 0;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cube);
    if (cube == 0)
        return 0;

    glTextureStorage2D(cube, kIblPrefilterMips, GL_RGB16F, kIblPrefilterSize, kIblPrefilterSize);
    glTextureParameteri(cube, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(cube, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(cube, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cube, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cube, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    GLuint fbo = 0;
    glCreateFramebuffers(1, &fbo);
    if (fbo == 0) {
        glDeleteTextures(1, &cube);
        return 0;
    }

    GLint vp[4] = {};
    glGetIntegerv(GL_VIEWPORT, vp);
    GLint prev_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDisable(GL_DEPTH_TEST);

    prefilter.bind();
    prefilter.setMatrix4("projectionMatrix", ibl_cube_proj());
    prefilter.setInt("envMap", 0);
    prefilter.setFloat("envResolution", float(src_size));
    prefilter.setInt("sampleCount", kIblPrefilterSamples);
    glBindTextureUnit(0, env_cube);

    if (!ibl_bind_cube_vao()) {
        glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
        glViewport(vp[0], vp[1], vp[2], vp[3]);
        glEnable(GL_DEPTH_TEST);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &cube);
        return 0;
    }

    const glm::mat4* views = ibl_cube_views();
    bool ok = true;
    for (int mip = 0; mip < kIblPrefilterMips && ok; ++mip) {
        const int mip_size = kIblPrefilterSize >> mip;
        const float roughness = float(mip) / kIblPrefilterMaxLod;
        glViewport(0, 0, mip_size, mip_size);
        prefilter.setFloat("roughness", roughness);
        for (int face = 0; face < 6 && ok; ++face) {
            prefilter.setMatrix4("viewMatrix", views[face]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, cube, mip);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                ok = false;
                break;
            }
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glEnable(GL_DEPTH_TEST);
    glDeleteFramebuffers(1, &fbo);

    if (!ok) {
        glDeleteTextures(1, &cube);
        return 0;
    }
    return cube;
}
