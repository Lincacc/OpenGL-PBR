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
#include <unordered_map>
#include <utility>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

inline void make_icosphere(std::vector<Vertex>& verts, std::vector<uint32_t>& indices)
{
    const float t = 1.61803398875f;
    const glm::vec3 raw[] = {
        { -1.f,  t, 0.f }, {  1.f,  t, 0.f }, { -1.f, -t, 0.f }, {  1.f, -t, 0.f },
        {  0.f, -1.f,  t }, {  0.f,  1.f,  t }, {  0.f, -1.f, -t }, {  0.f,  1.f, -t },
        {  t, 0.f, -1.f }, {  t, 0.f,  1.f }, { -t, 0.f, -1.f }, { -t, 0.f,  1.f },
    };
    std::vector<glm::vec3> pos;
    pos.reserve(42);
    for (const auto& p : raw)
        pos.push_back(glm::normalize(p));

    const uint32_t faces[][3] = {
        { 0, 11, 5 }, { 0, 5, 1 }, { 0, 1, 7 }, { 0, 7, 10 }, { 0, 10, 11 },
        { 1, 5, 9 }, { 5, 11, 4 }, { 11, 10, 2 }, { 10, 7, 6 }, { 7, 1, 8 },
        { 3, 9, 4 }, { 3, 4, 2 }, { 3, 2, 6 }, { 3, 6, 8 }, { 3, 8, 9 },
        { 4, 9, 5 }, { 2, 4, 11 }, { 6, 2, 10 }, { 8, 6, 7 }, { 9, 8, 1 },
    };

    std::unordered_map<uint64_t, uint32_t> mid;
    auto midpoint = [&](uint32_t a, uint32_t b) {
        if (a > b)
            std::swap(a, b);
        const uint64_t k = (uint64_t(a) << 32) | uint64_t(b);
        if (auto it = mid.find(k); it != mid.end())
            return it->second;
        const uint32_t id = uint32_t(pos.size());
        pos.push_back(glm::normalize(pos[a] + pos[b]));
        mid.emplace(k, id);
        return id;
    };

    indices.clear();
    indices.reserve(80 * 3);
    for (const auto& f : faces) {
        const uint32_t a = f[0], b = f[1], c = f[2];
        const uint32_t ab = midpoint(a, b);
        const uint32_t bc = midpoint(b, c);
        const uint32_t ca = midpoint(c, a);
        indices.insert(indices.end(), { a, ab, ca, b, bc, ab, c, ca, bc, ab, bc, ca });
    }

    verts.resize(pos.size());
    for (size_t i = 0; i < pos.size(); ++i)
        verts[i] = { .position = pos[i], .normal = pos[i], .uv = {} };
}

struct Actor {
    Shader shader;
    Shader shader1;
    Shader shader2;
    Shader shader2Depth;
    Shader shader3;
    Shader shader4;
    Shader shader5;
    Shader shader5Point;
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

    glm::mat3 mesh_rotation() const {
        return glm::mat3_cast(glm::normalize(meshOrient));
    }

    void bind_transform(Shader& sh) const {
        sh.setMatrix3("meshRotation", mesh_rotation());
    }

    void rotated_bounds(glm::vec3& bmin, glm::vec3& bmax) const {
        const glm::mat3 R = mesh_rotation();
        bmin = glm::vec3(1e9f);
        bmax = glm::vec3(-1e9f);
        for (const auto& pos : mesh.positions) {
            const glm::vec3 p = R * pos;
            bmin = glm::min(bmin, p);
            bmax = glm::max(bmax, p);
        }
    }

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
    GLuint pointVao = 0, sphereVbo = 0, sphereEbo = 0, instanceVbo = 0;
    GLsizei sphereIndexCount = 0;
    GLsizei instanceCount = 0;
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
        init_point_instances();
    }

    void init_point_instances() {
        std::vector<Vertex> sphereVerts;
        std::vector<uint32_t> sphereIdx;
        make_icosphere(sphereVerts, sphereIdx);

        std::vector<glm::vec3> inst = mesh.positions;
        std::sort(inst.begin(), inst.end(), [](const glm::vec3& a, const glm::vec3& b) {
            if (a.x != b.x) return a.x < b.x;
            if (a.y != b.y) return a.y < b.y;
            return a.z < b.z;
        });
        inst.erase(std::unique(inst.begin(), inst.end(), [](const glm::vec3& a, const glm::vec3& b) {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }), inst.end());
        if (inst.empty() || sphereVerts.empty() || sphereIdx.empty())
            throw std::runtime_error("Actor::init point instances empty");

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

        glCreateVertexArrays(1, &pointVao);
        glCreateBuffers(1, &sphereVbo);
        glCreateBuffers(1, &sphereEbo);
        glCreateBuffers(1, &instanceVbo);
        if (pointVao == 0 || sphereVbo == 0 || sphereEbo == 0 || instanceVbo == 0)
            throw std::runtime_error("Actor::init point glCreate* failed");

        glNamedBufferStorage(sphereVbo, sphereVerts.size() * sizeof(Vertex), sphereVerts.data(), 0);
        glNamedBufferStorage(sphereEbo, sphereIdx.size() * sizeof(uint32_t), sphereIdx.data(), 0);
        glNamedBufferStorage(instanceVbo, inst.size() * sizeof(glm::vec3), inst.data(), 0);

        glVertexArrayVertexBuffer(pointVao, 0, sphereVbo, 0, sizeof(Vertex));
        glVertexArrayVertexBuffer(pointVao, 1, instanceVbo, 0, sizeof(glm::vec3));
        glVertexArrayElementBuffer(pointVao, sphereEbo);
        glVertexArrayBindingDivisor(pointVao, 1, 1);

        glEnableVertexArrayAttrib(pointVao, 0);
        glVertexArrayAttribFormat(pointVao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
        glVertexArrayAttribBinding(pointVao, 0, 0);

        glEnableVertexArrayAttrib(pointVao, 1);
        glVertexArrayAttribFormat(pointVao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
        glVertexArrayAttribBinding(pointVao, 1, 0);

        glEnableVertexArrayAttrib(pointVao, 3);
        glVertexArrayAttribFormat(pointVao, 3, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(pointVao, 3, 1);

        sphereIndexCount = static_cast<GLsizei>(sphereIdx.size());
        instanceCount = static_cast<GLsizei>(inst.size());
    }

    void draw() {
        Shader& sh = active_shader();
        bind_transform(sh);
        if (meshInspect) {
            sh.setVec3f("albedo", material5.albedo);
            sh.setVec3f("rim", material5.rim);
            sh.setFloat("power", material5.power);
            if (meshDrawMode == 1)
                sh.setFloat("pointRadius", pointRadius);
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
            glBindVertexArray(pointVao);
            glDrawElementsInstanced(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, nullptr, instanceCount);
            glBindVertexArray(0);
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