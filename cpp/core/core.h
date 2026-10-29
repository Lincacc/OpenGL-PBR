#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>

#include "shader_program.h"
#include "obj_loader.h"

struct Usage {
    float cpu = .0f;
    float gpu = .0f;
};

static Usage host_usage() {
    static Usage usage;
    static LARGE_INTEGER freq{};
    static LARGE_INTEGER last_qpc{};
    static FILETIME last_idle{};
    static FILETIME last_kernel{};
    static FILETIME last_user{};
    static PDH_HQUERY query = nullptr;
    static PDH_HCOUNTER counter = nullptr;
    static bool ready = false;

    auto to_u64 = [](const FILETIME& ft) {
        return (ULONGLONG(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    if (!ready) {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&last_qpc);
        GetSystemTimes(&last_idle, &last_kernel, &last_user);
        if (PdhOpenQueryW(nullptr, 0, &query) == ERROR_SUCCESS &&
            PdhAddEnglishCounterW(query, L"\\GPU Engine(*)\\Utilization Percentage", 0, &counter) == ERROR_SUCCESS) {
            PdhCollectQueryData(query);
        }
        ready = true;
        return usage;
    }

    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    double elapsed = double(now.QuadPart - last_qpc.QuadPart) / double(freq.QuadPart);
    if (elapsed < 0.5)
        return usage;
    last_qpc = now;
    FILETIME idle{}, kernel{}, user{};
    GetSystemTimes(&idle, &kernel, &user);
    ULONGLONG d_idle = to_u64(idle) - to_u64(last_idle);
    ULONGLONG d_total = (to_u64(kernel) - to_u64(last_kernel)) + (to_u64(user) - to_u64(last_user));
    last_idle = idle;
    last_kernel = kernel;
    last_user = user;
    if (d_total > 0)
        usage.cpu = float(d_total - d_idle) * 100.f / float(d_total);
    if (query && PdhCollectQueryData(query) == ERROR_SUCCESS) {
        DWORD size = 0;
        DWORD count = 0;
        if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, nullptr) == PDH_MORE_DATA) {
            std::vector<unsigned char> buffer(size);
            auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
            if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &size, &count, items) == ERROR_SUCCESS) {
                double gpu = 0.0;
                for (DWORD i = 0; i < count; ++i) {
                    if (wcsstr(items[i].szName, L"engtype_3D") == nullptr)
                        continue;
                    if (items[i].FmtValue.CStatus != ERROR_SUCCESS)
                        continue;
                    if (items[i].FmtValue.doubleValue > gpu)
                        gpu = items[i].FmtValue.doubleValue;
                }
                usage.gpu = static_cast<float>(gpu);
            }
        }
    }
    return usage;
}

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Actor {
    Shader shader;
    ObjMesh mesh;

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;

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
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
};