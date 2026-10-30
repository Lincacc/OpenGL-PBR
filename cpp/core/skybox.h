#pragma once

#include "shader_program.h"
#include "shaders/shader_code_skybox.h"

#include <cstdint>
#include <stdexcept>

inline const float kUnitCubeTriangles[] = {
    -1.f,  1.f, -1.f,  -1.f, -1.f, -1.f,   1.f, -1.f, -1.f,
     1.f, -1.f, -1.f,   1.f,  1.f, -1.f,  -1.f,  1.f, -1.f,

    -1.f, -1.f,  1.f,  -1.f, -1.f, -1.f,  -1.f,  1.f, -1.f,
    -1.f,  1.f, -1.f,  -1.f,  1.f,  1.f,  -1.f, -1.f,  1.f,

     1.f, -1.f, -1.f,   1.f, -1.f,  1.f,   1.f,  1.f,  1.f,
     1.f,  1.f,  1.f,   1.f,  1.f, -1.f,   1.f, -1.f, -1.f,

    -1.f, -1.f,  1.f,  -1.f,  1.f,  1.f,   1.f,  1.f,  1.f,
     1.f,  1.f,  1.f,   1.f, -1.f,  1.f,  -1.f, -1.f,  1.f,

    -1.f,  1.f, -1.f,   1.f,  1.f, -1.f,   1.f,  1.f,  1.f,
     1.f,  1.f,  1.f,  -1.f,  1.f,  1.f,  -1.f,  1.f, -1.f,

    -1.f, -1.f, -1.f,  -1.f, -1.f,  1.f,   1.f, -1.f, -1.f,
     1.f, -1.f, -1.f,  -1.f, -1.f,  1.f,   1.f, -1.f,  1.f,
};

struct Skybox {
    Shader shader;
    GLuint vao = 0, vbo = 0;
    GLsizei vertexCount = 36;

    void init() {
        shader.compile(vSkyboxShaderCode, fSkyboxShaderCode);

        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        if (vao == 0 || vbo == 0)
            throw std::runtime_error("Skybox::init glCreate* failed");

        glNamedBufferStorage(vbo, sizeof(kUnitCubeTriangles), kUnitCubeTriangles, 0);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 3);
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }

    void draw(const glm::mat4& view, const glm::mat4& proj, GLuint envCube) {
        if (envCube == 0)
            return;

        shader.bind();
        shader.setMatrix4("viewMatrix", glm::mat4(glm::mat3(view)));
        shader.setMatrix4("projectionMatrix", proj);
        shader.setInt("envMap", 0);
        glBindTextureUnit(0, envCube);

        glDepthFunc(GL_LEQUAL);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }
};
