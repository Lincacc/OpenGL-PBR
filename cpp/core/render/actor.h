#pragma once

#include "core/shader_program.h"
#include "core/obj_loader.h"
#include "core/materials/material_0.h"
#include "core/materials/material_1.h"

#include <cstddef>
#include <stdexcept>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Actor {
    Shader shader;
    Shader shader1;
    ObjMesh mesh;
    Material material;
    Material1 material1;
    int materialId = 0;

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;

    Shader& active_shader() {
        return materialId == 0 ? shader : shader1;
    }

    void init() {
        if (mesh.positions.empty() || mesh.indices.empty()) {
            throw std::runtime_error("Actor::init mesh is empty");
        }

        if (mesh.positions.size() != mesh.normals.size() ||
            mesh.positions.size() != mesh.uvs.size()) {
            throw std::runtime_error("Actor::init attribute size mismatch");
        }

        std::vector<Vertex> vertices(mesh.positions.size());
        for (size_t i = 0; i < vertices.size(); ++i) {
            vertices[i] = {
                .position = mesh.positions[i],
                .normal = mesh.normals[i],
                .uv = mesh.uvs[i],
            };
        }
        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        glCreateBuffers(1, &ebo);

        if (vao == 0 || vbo == 0 || ebo == 0) {
            throw std::runtime_error("Actor::init glCreate* failed");
        }

        glNamedBufferStorage(vbo, vertices.size() * sizeof(Vertex), vertices.data(), 0);
        glNamedBufferStorage(ebo, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), 0);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(vao, ebo);

        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
        glVertexArrayAttribBinding(vao, 0, 0);

        glEnableVertexArrayAttrib(vao, 1);
        glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
        glVertexArrayAttribBinding(vao, 1, 0);

        glEnableVertexArrayAttrib(vao, 2);
        glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
        glVertexArrayAttribBinding(vao, 2, 0);

        indexCount = static_cast<GLsizei>(mesh.indices.size());
    }

    void draw() {
        Shader& sh = active_shader();
        if (materialId == 0) {
            sh.setVec3f("albedo", material.albedo);
            sh.setFloat("metallic", material.metallic);
            sh.setFloat("roughness", material.roughness);
        } else {
            sh.setVec3f("albedo", material1.albedo);
            sh.setFloat("metallic", material1.metallic);
            sh.setFloat("roughness", material1.roughness);
            sh.setFloat("clearcoat", material1.clearcoat);
            sh.setFloat("clearcoatRoughness", material1.clearcoatRoughness);
        }

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
};