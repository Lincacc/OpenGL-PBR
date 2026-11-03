#pragma once

#include "core/shader_program.h"
#include "core/obj_loader.h"
#include "core/materials/material_0.h"
#include "core/materials/material_1.h"
#include "core/materials/material_2.h"
#include "core/materials/material_3.h"
#include "core/materials/material_4.h"
#include "core/materials/material_5.h"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Actor {
    Shader shader;
    Shader shader1;
    Shader shader2;
    Shader shader2Depth;
    Shader shader3;
    Shader shader4;
    Shader shader5;
    Shader shader5Point;
    Shader shaderShadow;
    ObjMesh mesh;
    Material material;
    Material1 material1;
    Material2 material2;
    Material3 material3;
    Material4 material4;
    Material5 material5;
    int materialId = 0;
    bool meshInspect = false;
    int meshDrawMode = 0;
    glm::quat meshOrient{ 1.f, 0.f, 0.f, 0.f };
    glm::vec3 localMin{ 0.f };
    glm::vec3 localMax{ 0.f };

    glm::mat3 mesh_rotation() const {
        return glm::mat3_cast(glm::normalize(meshOrient));
    }

    void bind_transform(Shader& sh) const {
        sh.setMatrix3("meshRotation", mesh_rotation());
    }

    void update_local_bounds() {
        localMin = glm::vec3(1e9f);
        localMax = glm::vec3(-1e9f);
        for (const auto& pos : mesh.positions) {
            localMin = glm::min(localMin, pos);
            localMax = glm::max(localMax, pos);
        }
    }

    void rotated_bounds(glm::vec3& bmin, glm::vec3& bmax) const {
        const glm::mat3 R = mesh_rotation();
        bmin = glm::vec3(1e9f);
        bmax = glm::vec3(-1e9f);
        for (int i = 0; i < 8; ++i) {
            const glm::vec3 c(
                (i & 1) ? localMax.x : localMin.x,
                (i & 2) ? localMax.y : localMin.y,
                (i & 4) ? localMax.z : localMin.z);
            const glm::vec3 p = R * c;
            bmin = glm::min(bmin, p);
            bmax = glm::max(bmax, p);
        }
    }

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
    GLsizei vertexCount = 0;
    float pointRadius = 0.01f;

    Shader& active_shader() {
        if (meshInspect) {
            if (meshDrawMode == 1)
                return shader5Point;
            return shader5;
        }
        if (materialId == 1)
            return shader1;
        if (materialId == 2)
            return shader2;
        if (materialId == 3)
            return shader3;
        if (materialId == 4)
            return shader4;
        return shader;
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
        vertexCount = static_cast<GLsizei>(mesh.positions.size());
        update_local_bounds();
        update_point_radius();
    }

    void destroy_geometry() {
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (ebo != 0) {
            glDeleteBuffers(1, &ebo);
            ebo = 0;
        }
        indexCount = 0;
        vertexCount = 0;
    }

    void set_mesh(ObjMesh next) {
        ObjMesh prev = std::move(mesh);
        destroy_geometry();
        mesh = std::move(next);
        try {
            init();
        } catch (...) {
            destroy_geometry();
            mesh = std::move(prev);
            init();
            throw;
        }
    }

    void update_point_radius() {
        double acc = 0.0;
        size_t nedge = 0;
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const glm::vec3& p0 = mesh.positions[mesh.indices[i]];
            const glm::vec3& p1 = mesh.positions[mesh.indices[i + 1]];
            const glm::vec3& p2 = mesh.positions[mesh.indices[i + 2]];
            acc += double(glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p0 - p2));
            nedge += 3;
        }
        const float r = nedge > 0 ? 0.12f * float(acc / double(nedge)) : 0.006f;
        pointRadius = std::clamp(r, 0.002f, 0.025f);
    }

    void draw() {
        Shader& sh = active_shader();
        bind_transform(sh);
        if (meshInspect) {
            sh.setVec3f("albedo", material5.albedo);
            sh.setVec3f("rim", material5.rim);
            sh.setFloat("power", material5.power);
            if (meshDrawMode == 1) {
                GLint vp[4] = {};
                glGetIntegerv(GL_VIEWPORT, vp);
                sh.setFloat("pointRadius", pointRadius);
                sh.setFloat("viewportHeight", float(vp[3]));
            }
            draw_geometry();
            return;
        }
        if (materialId == 0) {
            sh.setVec3f("albedo", material.albedo);
            sh.setFloat("metallic", material.metallic);
            sh.setFloat("roughness", material.roughness);
        } else if (materialId == 1) {
            sh.setVec3f("albedo", material1.albedo);
            sh.setFloat("metallic", material1.metallic);
            sh.setFloat("roughness", material1.roughness);
            sh.setFloat("specular", material1.specular);
            sh.setFloat("clearcoat", material1.clearcoat);
            sh.setFloat("clearcoatRoughness", material1.clearcoatRoughness);
        } else if (materialId == 2) {
            sh.setVec3f("albedo", material2.albedo);
            sh.setFloat("roughness", material2.roughness);
            sh.setFloat("ior", material2.ior);
            sh.setFloat("transmission", material2.transmission);
            sh.setVec3f("attenuationColor", material2.attenuationColor);
            sh.setFloat("attenuationDistance", material2.attenuationDistance);
        } else if (materialId == 3) {
            sh.setVec3f("albedo", material3.albedo);
            sh.setFloat("metallic", material3.metallic);
            sh.setFloat("roughness", material3.roughness);
            sh.setFloat("specular", material3.specular);
            sh.setVec3f("subsurfaceColor", material3.subsurfaceColor);
            sh.setFloat("opacity", material3.opacity);
        } else if (materialId == 4) {
            sh.setVec3f("albedo", material4.albedo);
            sh.setFloat("metallic", material4.metallic);
            sh.setFloat("roughness", material4.roughness);
            sh.setFloat("specular", material4.specular);
            sh.setVec3f("fuzzColor", material4.fuzzColor);
            sh.setFloat("cloth", material4.cloth);
        }

        draw_geometry();
    }

    void draw_geometry() {
        if (meshInspect && meshDrawMode == 1) {
            glEnable(GL_PROGRAM_POINT_SIZE);
            glBindVertexArray(vao);
            glDrawArrays(GL_POINTS, 0, vertexCount);
            glBindVertexArray(0);
            glDisable(GL_PROGRAM_POINT_SIZE);
            return;
        }
        const bool wire = meshInspect && meshDrawMode == 2;
        if (wire)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        if (wire)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
};