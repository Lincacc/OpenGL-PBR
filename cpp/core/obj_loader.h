#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct ObjMesh {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;
};

namespace obj_detail {

constexpr char kMeshMagic[4] = { 'P', 'B', 'R', 'M' };
constexpr uint32_t kMeshVersion = 1;

inline void skip_ws(std::string_view& s) {
    while (!s.empty() && (s[0] == ' ' || s[0] == '\t'))
        s.remove_prefix(1);
}

inline bool cmd_eq(std::string_view line, std::string_view cmd) {
    if (line.size() < cmd.size())
        return false;
    if (line.compare(0, cmd.size(), cmd) != 0)
        return false;
    return line.size() == cmd.size() ||
        std::isspace(static_cast<unsigned char>(line[cmd.size()]));
}

inline float parse_f32(std::string_view& s, const char* what) {
    skip_ws(s);
    if (s.empty())
        throw std::runtime_error(std::string("obj missing ") + what);
    float v = 0.f;
    const auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{})
        throw std::runtime_error(std::string("obj bad ") + what);
    s.remove_prefix(static_cast<size_t>(p - s.data()));
    return v;
}

inline size_t resolve_index(int idx, size_t count, const char* name) {
    if (idx > 0 && static_cast<size_t>(idx) <= count)
        return static_cast<size_t>(idx - 1);
    if (idx < 0 && static_cast<size_t>(-idx) <= count)
        return count + static_cast<size_t>(idx);
    throw std::runtime_error(std::string("obj ") + name + " index out of range");
}

struct Corner {
    int v = 0;
    int t = 0;
    int n = 0;

    bool operator==(const Corner& o) const {
        return v == o.v && t == o.t && n == o.n;
    }
};

struct CornerHash {
    size_t operator()(const Corner& c) const noexcept {
        size_t h = std::hash<int>{}(c.v);
        h ^= std::hash<int>{}(c.t) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(c.n) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

inline Corner parse_corner(std::string_view tok) {
    Corner c{};
    auto next = [&](int& out) {
        if (tok.empty())
            return;
        const auto slash = tok.find('/');
        const auto part = tok.substr(0, slash);
        if (!part.empty()) {
            const auto [p, ec] = std::from_chars(part.data(), part.data() + part.size(), out);
            if (ec != std::errc{})
                throw std::runtime_error("bad obj face index");
        }
        tok = slash == std::string_view::npos ? std::string_view{} : tok.substr(slash + 1);
    };
    next(c.v);
    next(c.t);
    next(c.n);
    return c;
}

inline std::string read_all(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open obj: " + path.string());
    in.seekg(0, std::ios::end);
    const auto end = in.tellg();
    if (end < 0)
        throw std::runtime_error("cannot size obj: " + path.string());
    in.seekg(0, std::ios::beg);
    std::string buf(static_cast<size_t>(end), '\0');
    if (end > 0 && !in.read(buf.data(), end))
        throw std::runtime_error("cannot read obj: " + path.string());
    return buf;
}

template <typename Fn>
void for_each_line(std::string_view text, Fn&& fn) {
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.remove_prefix(3);
    }

    while (!text.empty()) {
        const auto nl = text.find('\n');
        std::string_view line = text.substr(0, nl);
        if (nl == std::string_view::npos)
            text = {};
        else
            text.remove_prefix(nl + 1);
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);
        skip_ws(line);
        fn(line);
    }
}

inline void center_and_scale(std::vector<glm::vec3>& positions) {
    if (positions.empty())
        throw std::runtime_error("obj has no positions");

    glm::vec3 center{ 0.f };
    for (const auto& p : positions)
        center += p;
    center /= float(positions.size());

    float max_len = 0.f;
    for (auto& p : positions) {
        p -= center;
        max_len = glm::max(max_len, glm::length(p));
    }
    if (max_len > 1e-8f) {
        for (auto& p : positions)
            p /= max_len;
    }
}

inline void generate_normals(
    ObjMesh& mesh,
    const std::vector<size_t>* src_pos,
    size_t raw_pos_count)
{
    if (src_pos) {
        std::vector<glm::vec3> acc(raw_pos_count, glm::vec3(0.f));
        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            const uint32_t i0 = mesh.indices[i];
            const uint32_t i1 = mesh.indices[i + 1];
            const uint32_t i2 = mesh.indices[i + 2];
            const glm::vec3 fn = glm::cross(
                mesh.positions[i1] - mesh.positions[i0],
                mesh.positions[i2] - mesh.positions[i0]);
            acc[(*src_pos)[i0]] += fn;
            acc[(*src_pos)[i1]] += fn;
            acc[(*src_pos)[i2]] += fn;
        }
        for (auto& n : acc) {
            const float len = glm::length(n);
            n = (len > 1e-8f) ? (n / len) : glm::vec3(0.f, 1.f, 0.f);
        }
        for (size_t i = 0; i < mesh.normals.size(); ++i)
            mesh.normals[i] = acc[(*src_pos)[i]];
        return;
    }

    std::fill(mesh.normals.begin(), mesh.normals.end(), glm::vec3(0.f));
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const uint32_t i0 = mesh.indices[i];
        const uint32_t i1 = mesh.indices[i + 1];
        const uint32_t i2 = mesh.indices[i + 2];
        const glm::vec3 fn = glm::cross(
            mesh.positions[i1] - mesh.positions[i0],
            mesh.positions[i2] - mesh.positions[i0]);
        mesh.normals[i0] += fn;
        mesh.normals[i1] += fn;
        mesh.normals[i2] += fn;
    }
    for (auto& n : mesh.normals) {
        const float len = glm::length(n);
        n = (len > 1e-8f) ? (n / len) : glm::vec3(0.f, 1.f, 0.f);
    }
}

inline ObjMesh parse_obj_text(const std::filesystem::path& path) {
    const std::string text = read_all(path);

    size_t n_v = 0, n_vt = 0, n_vn = 0, n_f = 0;
    for_each_line(text, [&](std::string_view line) {
        if (line.empty() || line[0] == '#')
            return;
        if (cmd_eq(line, "v"))
            ++n_v;
        else if (cmd_eq(line, "vt"))
            ++n_vt;
        else if (cmd_eq(line, "vn"))
            ++n_vn;
        else if (cmd_eq(line, "f"))
            ++n_f;
    });

    std::vector<glm::vec3> raw_pos;
    std::vector<glm::vec3> raw_nrm;
    std::vector<glm::vec2> raw_uv;
    raw_pos.reserve(n_v);
    raw_nrm.reserve(n_vn);
    raw_uv.reserve(n_vt);

    std::vector<Corner> corners;
    corners.reserve(n_f * 3);
    bool used_vt = false;
    bool used_vn = false;

    for_each_line(text, [&](std::string_view line) {
        if (line.empty() || line[0] == '#')
            return;

        if (cmd_eq(line, "v")) {
            line.remove_prefix(1);
            glm::vec3 p{};
            p.x = parse_f32(line, "v.x");
            p.y = parse_f32(line, "v.y");
            p.z = parse_f32(line, "v.z");
            raw_pos.push_back(p);
            return;
        }
        if (cmd_eq(line, "vt")) {
            line.remove_prefix(2);
            glm::vec2 t{};
            t.x = parse_f32(line, "vt.x");
            t.y = parse_f32(line, "vt.y");
            raw_uv.push_back(t);
            return;
        }
        if (cmd_eq(line, "vn")) {
            line.remove_prefix(2);
            glm::vec3 n{};
            n.x = parse_f32(line, "vn.x");
            n.y = parse_f32(line, "vn.y");
            n.z = parse_f32(line, "vn.z");
            raw_nrm.push_back(n);
            return;
        }
        if (!cmd_eq(line, "f"))
            return;

        line.remove_prefix(1);
        Corner face[32];
        int nvert = 0;
        while (true) {
            skip_ws(line);
            if (line.empty())
                break;
            if (nvert >= 32)
                throw std::runtime_error("obj face has more than 32 vertices");
            auto end = line.find_first_of(" \t");
            const auto tok = line.substr(0, end);
            face[nvert] = parse_corner(tok);
            if (face[nvert].t != 0)
                used_vt = true;
            if (face[nvert].n != 0)
                used_vn = true;
            ++nvert;
            if (end == std::string_view::npos)
                break;
            line.remove_prefix(end);
        }
        if (nvert < 3)
            throw std::runtime_error("obj face needs at least 3 vertices");
        for (int i = 1; i + 1 < nvert; ++i) {
            corners.push_back(face[0]);
            corners.push_back(face[i]);
            corners.push_back(face[i + 1]);
        }
    });

    if (corners.empty())
        throw std::runtime_error("obj has no triangles: " + path.string());

    ObjMesh mesh;
    const bool v_only = !used_vt && !used_vn;

    if (v_only) {
        mesh.positions = std::move(raw_pos);
        mesh.normals.assign(mesh.positions.size(), glm::vec3(0.f));
        mesh.uvs.assign(mesh.positions.size(), glm::vec2(0.f));
        mesh.indices.reserve(corners.size());
        for (const auto& c : corners) {
            if (c.v == 0)
                throw std::runtime_error("obj face missing vertex index");
            mesh.indices.push_back(static_cast<uint32_t>(
                resolve_index(c.v, mesh.positions.size(), "v")));
        }
        center_and_scale(mesh.positions);
        generate_normals(mesh, nullptr, 0);
        return mesh;
    }

    std::unordered_map<Corner, uint32_t, CornerHash> unique;
    unique.reserve(corners.size());
    std::vector<size_t> src_pos;
    src_pos.reserve(corners.size());
    mesh.positions.reserve(corners.size());
    mesh.normals.reserve(corners.size());
    mesh.uvs.reserve(corners.size());
    mesh.indices.reserve(corners.size());

    for (const auto& c : corners) {
        if (auto it = unique.find(c); it != unique.end()) {
            mesh.indices.push_back(it->second);
            continue;
        }

        glm::vec3 pos{};
        glm::vec3 nrm{};
        glm::vec2 uv{};
        size_t pos_i = 0;
        if (c.v != 0) {
            pos_i = resolve_index(c.v, raw_pos.size(), "v");
            pos = raw_pos[pos_i];
        }
        if (c.t != 0)
            uv = raw_uv[resolve_index(c.t, raw_uv.size(), "vt")];
        if (c.n != 0)
            nrm = raw_nrm[resolve_index(c.n, raw_nrm.size(), "vn")];

        const uint32_t id = static_cast<uint32_t>(mesh.positions.size());
        mesh.positions.push_back(pos);
        mesh.normals.push_back(nrm);
        mesh.uvs.push_back(uv);
        src_pos.push_back(pos_i);
        unique.emplace(c, id);
        mesh.indices.push_back(id);
    }

    center_and_scale(mesh.positions);
    if (!used_vn)
        generate_normals(mesh, &src_pos, raw_pos.size());
    return mesh;
}

inline ObjMesh load_mesh_cache(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open mesh cache: " + path.string());

    char magic[4]{};
    uint32_t version = 0;
    uint32_t nvert = 0;
    uint32_t nidx = 0;
    in.read(magic, 4);
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&nvert), sizeof(nvert));
    in.read(reinterpret_cast<char*>(&nidx), sizeof(nidx));
    if (!in || std::memcmp(magic, kMeshMagic, 4) != 0 || version != kMeshVersion)
        throw std::runtime_error("invalid mesh cache (delete it and reload): " + path.string());
    if (nvert == 0 || nidx == 0 || (nidx % 3) != 0)
        throw std::runtime_error("empty or untriangulated mesh cache: " + path.string());

    const auto file_size = std::filesystem::file_size(path);
    const uint64_t expected =
        16ull +
        uint64_t(nvert) * (sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2)) +
        uint64_t(nidx) * sizeof(uint32_t);
    if (file_size != expected)
        throw std::runtime_error("mesh cache size mismatch (delete it and reload): " + path.string());

    ObjMesh mesh;
    mesh.positions.resize(nvert);
    mesh.normals.resize(nvert);
    mesh.uvs.resize(nvert);
    mesh.indices.resize(nidx);
    in.read(reinterpret_cast<char*>(mesh.positions.data()), static_cast<std::streamsize>(nvert * sizeof(glm::vec3)));
    in.read(reinterpret_cast<char*>(mesh.normals.data()), static_cast<std::streamsize>(nvert * sizeof(glm::vec3)));
    in.read(reinterpret_cast<char*>(mesh.uvs.data()), static_cast<std::streamsize>(nvert * sizeof(glm::vec2)));
    in.read(reinterpret_cast<char*>(mesh.indices.data()), static_cast<std::streamsize>(nidx * sizeof(uint32_t)));
    if (!in)
        throw std::runtime_error("truncated mesh cache: " + path.string());

    for (uint32_t i : mesh.indices) {
        if (i >= nvert)
            throw std::runtime_error("mesh cache index out of range: " + path.string());
    }
    return mesh;
}

inline void save_mesh_cache(const std::filesystem::path& path, const ObjMesh& mesh) {
    if (mesh.positions.size() != mesh.normals.size() ||
        mesh.positions.size() != mesh.uvs.size()) {
        throw std::runtime_error("mesh attribute size mismatch, cannot cache: " + path.string());
    }
    if (mesh.positions.empty() || mesh.indices.empty() || (mesh.indices.size() % 3) != 0)
        throw std::runtime_error("cannot cache empty mesh: " + path.string());

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("cannot write mesh cache: " + path.string());

    const uint32_t nvert = static_cast<uint32_t>(mesh.positions.size());
    const uint32_t nidx = static_cast<uint32_t>(mesh.indices.size());
    out.write(kMeshMagic, 4);
    out.write(reinterpret_cast<const char*>(&kMeshVersion), sizeof(kMeshVersion));
    out.write(reinterpret_cast<const char*>(&nvert), sizeof(nvert));
    out.write(reinterpret_cast<const char*>(&nidx), sizeof(nidx));
    out.write(reinterpret_cast<const char*>(mesh.positions.data()),
        static_cast<std::streamsize>(nvert * sizeof(glm::vec3)));
    out.write(reinterpret_cast<const char*>(mesh.normals.data()),
        static_cast<std::streamsize>(nvert * sizeof(glm::vec3)));
    out.write(reinterpret_cast<const char*>(mesh.uvs.data()),
        static_cast<std::streamsize>(nvert * sizeof(glm::vec2)));
    out.write(reinterpret_cast<const char*>(mesh.indices.data()),
        static_cast<std::streamsize>(nidx * sizeof(uint32_t)));
    if (!out)
        throw std::runtime_error("failed writing mesh cache: " + path.string());
}

} // namespace obj_detail

inline ObjMesh load_obj(const std::filesystem::path& path) {
    auto ext = path.extension().string();
    for (char& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (ext != ".obj")
        throw std::runtime_error("only .obj is supported: " + path.string());

    std::filesystem::path cache = path;
    cache.replace_extension(".mesh");

    if (std::filesystem::exists(cache)) {
        if (!std::filesystem::exists(path))
            throw std::runtime_error("obj missing, cannot validate cache: " + path.string());
        if (std::filesystem::last_write_time(cache) >= std::filesystem::last_write_time(path))
            return obj_detail::load_mesh_cache(cache);
    }

    ObjMesh mesh = obj_detail::parse_obj_text(path);
    obj_detail::save_mesh_cache(cache, mesh);
    return mesh;
}
