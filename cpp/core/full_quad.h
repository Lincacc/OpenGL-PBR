#pragma once

#include <glad/glad.h>

#include <array>
#include <stdexcept>

struct FullscreenQuad {
    GLuint vao = 0;
    GLuint vbo = 0;

    void destroy() {
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
    }

    void create() {
        if (vao != 0 && vbo != 0)
            return;

        destroy();

        constexpr std::array<float, 18> vertices = {
            -1.f, -1.f, 0.f,
             1.f, -1.f, 0.f,
             1.f,  1.f, 0.f,
            -1.f, -1.f, 0.f,
             1.f,  1.f, 0.f,
            -1.f,  1.f, 0.f,
        };

        glCreateBuffers(1, &vbo);
        glCreateVertexArrays(1, &vao);
        if (vao == 0 || vbo == 0) {
            destroy();
            throw std::runtime_error("FullscreenQuad::create glCreate* failed");
        }

        glNamedBufferStorage(vbo, static_cast<GLsizeiptr>(sizeof(vertices)), vertices.data(), 0);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, 3 * sizeof(float));
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }

    void draw() const {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
};
