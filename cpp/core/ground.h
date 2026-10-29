#pragma once

#include "shader_program.h"
#include "shaders/shader_code_plan.h"

#include <cstdint>
#include <stdexcept>

struct Ground {
    Shader shader;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 6;
    float aoStrength = 0.7f;
    float fadeRadius = 3.f;

    void init(float floorY, float halfExtent) {
        shader.compile(vGroundShaderCode, fGroundShaderCode);

        const float s = halfExtent;
        const float y = floorY;
        const float verts[] = {
            -s, y, -s,
             s, y, -s,
             s, y,  s,
            -s, y,  s,
        };
        const uint32_t idx[] = { 0, 3, 2, 0, 2, 1 };

        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        glCreateBuffers(1, &ebo);
        if (vao == 0 || vbo == 0 || ebo == 0) {
            throw std::runtime_error("Ground::init glCreate* failed");
        }

        glNamedBufferStorage(vbo, sizeof(verts), verts, 0);
        glNamedBufferStorage(ebo, sizeof(idx), idx, 0);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 3);
        glVertexArrayElementBuffer(vao, ebo);
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }

    void draw(const glm::mat4& view, const glm::mat4& proj,
        const glm::vec3& ambient, const glm::vec2& aoCenter, float aoRadius)
    {
        shader.bind();
        shader.setMatrix4("viewMatrix", view);
        shader.setMatrix4("projectionMatrix", proj);
        shader.setVec3f("ambientColor", ambient);
        shader.setVec2f("aoCenter", aoCenter);
        shader.setFloat("aoRadius", aoRadius);
        shader.setFloat("aoStrength", aoStrength);
        shader.setFloat("fadeRadius", fadeRadius);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }
};
