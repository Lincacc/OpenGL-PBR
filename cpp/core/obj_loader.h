#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#include <cctype>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

struct ObjMesh {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;
};

inline ObjMesh load_obj(const std::filesystem::path& path) {
    auto ext = path.extension().string();
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (ext != ".obj") {
        throw std::runtime_error("only .obj is supported: " + path.string());
    }

    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open obj: " + path.string());
    }

    std::vector<glm::vec3> raw_pos;
    std::vector<glm::vec3> raw_nrm;
    std::vector<glm::vec2> raw_uv;

    auto parse_index = [](std::string_view tok) {
        int v = 0, t = 0, n = 0;
        auto next = [&](int& out) {
            if (tok.empty())
                return;
            auto slash = tok.find('/');
            auto part = tok.substr(0, slash);
            if (!part.empty()) {
                auto [p, ec] = std::from_chars(part.data(), part.data() + part.size(), out);
                if (ec != std::errc{})
                    throw std::runtime_error("bad obj face index");
            }
            tok = slash == std::string_view::npos ? std::string_view{} : tok.substr(slash + 1);
            };
        next(v);
        next(t);
        next(n);
        return std::tuple{ v, t, n };
    };

    auto resolve = [](int idx, size_t count, const char* name) -> size_t {
        if (idx > 0 && static_cast<size_t>(idx) <= count) {
            return static_cast<size_t>(idx - 1);
        }

        if (idx < 0 && static_cast<size_t>(-idx) <= count) {
            return count + static_cast<size_t>(idx);
        }
        throw std::runtime_error(std::string("obj ") + name + " index out of range");
    };

    ObjMesh mesh;
    std::map<std::tuple<int, int, int>, uint32_t> unique;
    std::vector<size_t> src_pos;
    bool referenced_vn = false;

    auto add_vert = [&](std::string_view tok) -> uint32_t {
        auto [vi, ti, ni] = parse_index(tok);
        if (auto it = unique.find({ vi, ti, ni }); it != unique.end())
            return it->second;
        glm::vec3 pos{};
        glm::vec3 nrm{};
        glm::vec2 uv{};
        size_t pos_i = 0;
        if (vi != 0) {
            pos_i = resolve(vi, raw_pos.size(), "v");
            pos = raw_pos[pos_i];
        }
        if (ti != 0)
            uv = raw_uv[resolve(ti, raw_uv.size(), "vt")];
        if (ni != 0) {
            referenced_vn = true;
            nrm = raw_nrm[resolve(ni, raw_nrm.size(), "vn")];
        }
        uint32_t id = static_cast<uint32_t>(mesh.positions.size());
        mesh.positions.push_back(pos);
        mesh.normals.push_back(nrm);
        mesh.uvs.push_back(uv);
        src_pos.push_back(pos_i);
        unique.emplace(std::tuple{ vi, ti, ni }, id);
        return id;
    };

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag == "v") {
            glm::vec3 p{};
            ss >> p.x >> p.y >> p.z;
            raw_pos.push_back(p);
        }
        else if (tag == "vt") {
            glm::vec2 t{};
            ss >> t.x >> t.y;
            raw_uv.push_back(t);
        }
        else if (tag == "vn") {
            glm::vec3 n{};
            ss >> n.x >> n.y >> n.z;
            raw_nrm.push_back(n);
        }
        else if (tag == "f") {
            std::vector<uint32_t> face;
            std::string tok;
            while (ss >> tok)
                face.push_back(add_vert(tok));
            if (face.size() < 3)
                throw std::runtime_error("obj face needs at least 3 vertices");
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                mesh.indices.push_back(face[0]);
                mesh.indices.push_back(face[i]);
                mesh.indices.push_back(face[i + 1]);
            }
        }
    }

    if (mesh.indices.empty()) {
        throw std::runtime_error("obj has no triangles: " + path.string());
    }

    glm::vec3 center{ 0.f };
    for (const auto& p : mesh.positions) {
        center += p;
    }

    center /= float(mesh.positions.size());
    float max_len = 0.f;
    for (auto& p : mesh.positions) {
        p -= center;
        max_len = glm::max(max_len, glm::length(p));
    }

    if (max_len > 1e-8f) {
        for (auto& p : mesh.positions)
            p /= max_len;
    }

    if (!referenced_vn) {
        std::vector<glm::vec3> acc(raw_pos.size(), glm::vec3(0.f));
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const uint32_t i0 = mesh.indices[i];
            const uint32_t i1 = mesh.indices[i + 1];
            const uint32_t i2 = mesh.indices[i + 2];
            const glm::vec3 fn = glm::cross(
                mesh.positions[i1] - mesh.positions[i0],
                mesh.positions[i2] - mesh.positions[i0]);
            acc[src_pos[i0]] += fn;
            acc[src_pos[i1]] += fn;
            acc[src_pos[i2]] += fn;
        }
        for (auto& n : acc) {
            const float len = glm::length(n);
            n = (len > 1e-8f) ? (n / len) : glm::vec3(0.f, 1.f, 0.f);
        }
        for (size_t i = 0; i < mesh.normals.size(); ++i)
            mesh.normals[i] = acc[src_pos[i]];
    }

    return mesh;
}