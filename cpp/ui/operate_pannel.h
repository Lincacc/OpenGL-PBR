#pragma once

#include "core/render/hdr_preview.h"
#include "core/render/ssao.h"
#include "core/light.h"
#include "core/materials/material_0.h"
#include "core/materials/material_1.h"
#include "core/materials/material_2.h"
#include "core/materials/material_3.h"
#include "core/materials/material_4.h"
#include "core/obj_loader.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

struct LightGizmo {
    float yaw = 0.785398f;
    float pitch = 0.615480f;
};

struct PreviewLight {
    bool enabled = false;
    int space = 0;
    float hue = 30.f;
    float sat = 0.7f;
    float val = 1.f;
    float intensity = 1.f;
};

struct HeaderIndent {
    HeaderIndent() { ImGui::Indent(); }
    ~HeaderIndent() { ImGui::Unindent(); }
    HeaderIndent(const HeaderIndent&) = delete;
    HeaderIndent& operator=(const HeaderIndent&) = delete;
};

inline LightGizmo light_gizmo_from_dir(const glm::vec3& dir) {
    glm::vec3 d = glm::normalize(dir);
    LightGizmo g{};
    g.pitch = std::asin(std::clamp(d.y, -1.f, 1.f));
    g.yaw = std::atan2(d.x, d.z);
    return g;
}

inline glm::vec3 light_dir_from_gizmo(const LightGizmo& g) {
    const float cp = std::cos(g.pitch);
    return glm::normalize(glm::vec3(cp * std::sin(g.yaw), std::sin(g.pitch), cp * std::cos(g.yaw)));
}

inline void draw_sun_icon(ImDrawList* dl, ImVec2 c, float r, ImU32 col) {
    dl->AddCircleFilled(c, r, col, 12);
    for (int i = 0; i < 8; ++i) {
        const float a = float(i) * (glm::pi<float>() * 0.25f);
        const ImVec2 d(std::cos(a), std::sin(a));
        dl->AddLine(
            ImVec2(c.x + d.x * (r + 1.5f), c.y + d.y * (r + 1.5f)),
            ImVec2(c.x + d.x * (r + 6.f), c.y + d.y * (r + 6.f)),
            col, 1.6f);
    }
}

inline ImU32 color_u32(const glm::vec3& c, int a) {
    auto u8 = [](float x) {
        return int(std::clamp(x, 0.f, 1.f) * 255.f + 0.5f);
        };
    return IM_COL32(u8(c.x), u8(c.y), u8(c.z), a);
}

inline glm::vec3 hsv_to_rgb(float hue_deg, float s, float v) {
    const float h = std::fmod(std::fmod(hue_deg, 360.f) + 360.f, 360.f) / 60.f;
    const float c = v * s;
    const float x = c * (1.f - std::fabs(std::fmod(h, 2.f) - 1.f));
    const float m = v - c;
    glm::vec3 rgb(0.f);
    if (h < 1.f) rgb = { c, x, 0.f };
    else if (h < 2.f) rgb = { x, c, 0.f };
    else if (h < 3.f) rgb = { 0.f, c, x };
    else if (h < 4.f) rgb = { 0.f, x, c };
    else if (h < 5.f) rgb = { x, 0.f, c };
    else rgb = { c, 0.f, x };
    return rgb + glm::vec3(m);
}

inline void rgb_to_hsv(const glm::vec3& rgb, float& hue, float& sat, float& val) {
    const float r = std::clamp(rgb.x, 0.f, 1.f);
    const float g = std::clamp(rgb.y, 0.f, 1.f);
    const float b = std::clamp(rgb.z, 0.f, 1.f);
    const float mx = std::max(r, std::max(g, b));
    const float mn = std::min(r, std::min(g, b));
    const float d = mx - mn;
    val = mx;
    sat = mx > 1e-6f ? d / mx : 0.f;
    if (d < 1e-6f)
        return;
    if (mx == r)
        hue = 60.f * (g - b) / d + (g < b ? 360.f : 0.f);
    else if (mx == g)
        hue = 60.f * (b - r) / d + 120.f;
    else
        hue = 60.f * (r - g) / d + 240.f;
}

inline void light_yaw_ring(const char* id, LightGizmo& g, PreviewLight& preview, float radius)
{
    ImGui::PushID(id);
    const float box = radius * 2.f + 28.f;
    const float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, (avail - box) * 0.5f));

    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("yaw_ring", ImVec2(box, box));
    const ImVec2 center(cursor.x + box * 0.5f, cursor.y + box * 0.5f);

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const float mx = mouse.x - center.x;
    const float my = mouse.y - center.y;
    const float md = std::sqrt(mx * mx + my * my);
    const float core_r = radius * 0.62f;
    const bool on_ring = std::abs(md - radius) <= 8.f;
    const bool on_core = md <= core_r;

    static bool dragging = false;
    if (ImGui::IsItemActivated()) {
        if (on_ring)
            dragging = true;
        else if (on_core)
            ImGui::OpenPopup("sun_color");
    }
    if (!ImGui::IsItemActive())
        dragging = false;
    if (ImGui::IsItemActive() && dragging && md > 1.f)
        g.yaw = std::atan2(mx, -my);

    glm::vec3 rgb = hsv_to_rgb(preview.hue, preview.sat, preview.val);
    if (ImGui::BeginPopup("sun_color")) {
        if (ImGui::ColorPicker3("##sun_color", &rgb.x,
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoSidePreview))
            rgb_to_hsv(rgb, preview.hue, preview.sat, preview.val);
        ImGui::EndPopup();
    }

    const bool hot = dragging || on_ring;
    const ImU32 ring_col = hot ? IM_COL32(255, 210, 40, 255) : IM_COL32(70, 150, 255, 255);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(center, core_r, ImGui::GetColorU32(ImVec4(0.729f, 0.741f, 0.764f, 1.f)), 32);
    dl->AddCircle(center, radius, ring_col, 64, hot ? 3.2f : 2.f);
    draw_sun_icon(dl,
        ImVec2(center.x + std::sin(g.yaw) * radius, center.y - std::cos(g.yaw) * radius),
        5.f, color_u32(glm::clamp(rgb, 0.f, 1.f), 255));

    ImGui::PopID();
}

struct ViewOrbitAnim {
    float t = 1.f;
    float yawFrom = 0.f;
    float yawTo = 0.f;
    float pitchFrom = 0.f;
    float pitchTo = 0.f;
};

struct MeshOrientAnim {
    float t = 1.f;
    glm::quat from{ 1.f, 0.f, 0.f, 0.f };
    glm::quat to{ 1.f, 0.f, 0.f, 0.f };
};

inline float wrap_pi(float a)
{
    const float pi = glm::pi<float>();
    while (a > pi) a -= 2.f * pi;
    while (a < -pi) a += 2.f * pi;
    return a;
}

inline float smootherstep(float t)
{
    t = std::clamp(t, 0.f, 1.f);
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

inline glm::vec3 orbit_dir(float yaw, float pitch)
{
    const float cp = std::cos(pitch);
    return glm::normalize(glm::vec3(cp * std::cos(yaw), std::sin(pitch), cp * std::sin(yaw)));
}

inline void dir_to_orbit(const glm::vec3& d, float& yaw, float& pitch)
{
    const glm::vec3 n = glm::normalize(d);
    pitch = std::clamp(std::asin(std::clamp(n.y, -1.f, 1.f)), -1.55f, 1.55f);
    yaw = std::atan2(n.z, n.x);
}

inline void begin_view_orbit(ViewOrbitAnim& a, float cy, float cp, const glm::vec3& dir)
{
    float ty = 0.f;
    float tp = 0.f;
    dir_to_orbit(dir, ty, tp);
    const float dy = wrap_pi(ty - cy);
    if (std::fabs(dy) < 1e-3f && std::fabs(tp - cp) < 1e-3f) {
        a.t = 1.f;
        return;
    }
    a.yawFrom = cy;
    a.yawTo = cy + dy;
    a.pitchFrom = cp;
    a.pitchTo = tp;
    a.t = 0.f;
}

inline void tick_view_orbit(ViewOrbitAnim& a, float& yaw, float& pitch, float dt)
{
    if (a.t >= 1.f)
        return;
    a.t = std::min(1.f, a.t + dt / 0.35f);
    const float w = smootherstep(a.t);
    yaw = glm::mix(a.yawFrom, a.yawTo, w);
    pitch = glm::mix(a.pitchFrom, a.pitchTo, w);
}

inline void begin_mesh_orient(MeshOrientAnim& a, glm::quat& q, const glm::vec3& axis, float radians)
{
    const glm::quat start = glm::normalize(q);
    const glm::quat base = a.t < 1.f ? a.to : start;
    a.from = start;
    a.to = glm::normalize(glm::angleAxis(radians, axis) * base);
    a.t = 0.f;
}

inline void tick_mesh_orient(MeshOrientAnim& a, glm::quat& q, float dt)
{
    if (a.t >= 1.f)
        return;
    a.t = std::min(1.f, a.t + dt / 0.35f);
    q = glm::normalize(glm::slerp(a.from, a.to, smootherstep(a.t)));
}

inline bool point_in_poly(ImVec2 p, const ImVec2* pts, int n)
{
    auto cross = [](ImVec2 o, ImVec2 u, ImVec2 v) {
        return (u.x - o.x) * (v.y - o.y) - (u.y - o.y) * (v.x - o.x);
    };
    bool pos = true;
    bool neg = true;
    for (int i = 0; i < n; ++i) {
        const float c = cross(p, pts[i], pts[(i + 1) % n]);
        pos = pos && c >= 0.f;
        neg = neg && c <= 0.f;
    }
    return pos || neg;
}

inline float dist_to_seg(ImVec2 p, ImVec2 a, ImVec2 b)
{
    const float vx = b.x - a.x;
    const float vy = b.y - a.y;
    const float len2 = vx * vx + vy * vy;
    float t = 0.f;
    if (len2 > 1e-6f)
        t = std::clamp(((p.x - a.x) * vx + (p.y - a.y) * vy) / len2, 0.f, 1.f);
    const float dx = p.x - (a.x + vx * t);
    const float dy = p.y - (a.y + vy * t);
    return std::sqrt(dx * dx + dy * dy);
}

inline void draw_world_axes(const glm::mat4& view)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float len = 42.f;
    const float pad = 20.f;
    const ImVec2 o(pad + len, io.DisplaySize.y - pad - len);
    const glm::mat3 R(view);

    auto proj = [&](const glm::vec3& w) {
        const glm::vec3 vs = R * w;
        return ImVec2(o.x + vs.x * len, o.y - vs.y * len);
    };

    struct Axis {
        glm::vec3 d;
        ImU32 col;
        const char* lab;
        float depth;
    };
    Axis ax[3] = {
        { { 1.f, 0.f, 0.f }, IM_COL32(210, 72, 72, 255), "X", 0.f },
        { { 0.f, 1.f, 0.f }, IM_COL32(72, 168, 86, 255), "Y", 0.f },
        { { 0.f, 0.f, 1.f }, IM_COL32(70, 118, 214, 255), "Z", 0.f },
    };
    for (int i = 0; i < 3; ++i)
        ax[i].depth = (R * ax[i].d).z;
    std::sort(ax, ax + 3, [](const Axis& a, const Axis& b) { return a.depth > b.depth; });

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    dl->AddCircleFilled(o, 3.2f, IM_COL32(80, 80, 84, 255), 12);
    for (const auto& a : ax) {
        const ImVec2 tip = proj(a.d);
        const ImVec2 lab = proj(a.d * 1.28f);
        dl->AddLine(o, tip, a.col, 2.4f);
        const ImVec2 ts = ImGui::CalcTextSize(a.lab);
        dl->AddText(ImVec2(lab.x - ts.x * 0.5f, lab.y - ts.y * 0.5f), a.col, a.lab);
    }
}

inline void draw_mesh_view_cube(float& orbitYaw, float& orbitPitch, ViewOrbitAnim& anim)
{
    const float avail = ImGui::GetContentRegionAvail().x;
    const float box = std::clamp(avail * 0.48f, 108.f, 136.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, (avail - box) * 0.5f));

    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("mesh_view_cube", ImVec2(box, box));
    const ImVec2 center(cursor.x + box * 0.5f, cursor.y + box * 0.5f);
    const float scale = box * 0.26f;
    const float handleR = 4.5f;
    const glm::vec3 eye = orbit_dir(orbitYaw, orbitPitch);
    glm::vec3 up(0.f, 1.f, 0.f);
    if (std::abs(glm::dot(eye, up)) > 0.995f)
        up = glm::vec3(-std::sin(orbitYaw), 0.f, std::cos(orbitYaw));
    const glm::vec3 fwd = -eye;
    glm::vec3 right = glm::cross(fwd, up);
    if (glm::dot(right, right) < 1e-8f)
        right = glm::vec3(1.f, 0.f, 0.f);
    right = glm::normalize(right);
    up = glm::cross(right, fwd);

    auto proj = [&](const glm::vec3& p) {
        return ImVec2(center.x + glm::dot(p, right) * scale, center.y - glm::dot(p, up) * scale);
    };

    glm::vec3 vtx[8];
    ImVec2 pv[8];
    for (int i = 0; i < 8; ++i) {
        vtx[i] = glm::vec3((i & 1) ? 1.f : -1.f, (i & 2) ? 1.f : -1.f, (i & 4) ? 1.f : -1.f);
        pv[i] = proj(vtx[i]);
    }

    const int faceV[6][4] = {
        { 1, 3, 7, 5 }, { 0, 4, 6, 2 },
        { 2, 3, 7, 6 }, { 0, 1, 5, 4 },
        { 4, 5, 7, 6 }, { 0, 2, 3, 1 },
    };
    const glm::vec3 faceN[6] = {
        { 1.f, 0.f, 0.f }, { -1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f }, { 0.f, -1.f, 0.f },
        { 0.f, 0.f, 1.f }, { 0.f, 0.f, -1.f },
    };
    const char* faceL[6] = {
        "\xE5\x8F\xB3", "\xE5\xB7\xA6",
        "\xE4\xB8\x8A", "\xE4\xB8\x8B",
        "\xE5\x89\x8D", "\xE5\x90\x8E",
    };
    const int edgeV[12][2] = {
        { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
        { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
    };

    float faceF[6];
    int faceOrder[6] = { 0, 1, 2, 3, 4, 5 };
    for (int i = 0; i < 6; ++i)
        faceF[i] = glm::dot(faceN[i], eye);
    std::sort(faceOrder, faceOrder + 6, [&](int a, int b) { return faceF[a] < faceF[b]; });

    int front = 0;
    for (int i = 1; i < 6; ++i) {
        if (faceF[i] > faceF[front])
            front = i;
    }
    const bool ortho = faceF[front] > 0.92f;

    ImVec2 ncirc[4]{};
    glm::vec3 ncircDir[4]{};
    int ncircN = 0;
    if (ortho) {
        for (int i = 0; i < 6; ++i) {
            if (i == front || i == (front ^ 1))
                continue;
            int sh[2] = { -1, -1 };
            int ns = 0;
            for (int a = 0; a < 4; ++a) {
                for (int b = 0; b < 4; ++b) {
                    if (faceV[front][a] == faceV[i][b] && ns < 2)
                        sh[ns++] = faceV[front][a];
                }
            }
            if (ns < 2)
                continue;
            const ImVec2 mid((pv[sh[0]].x + pv[sh[1]].x) * 0.5f, (pv[sh[0]].y + pv[sh[1]].y) * 0.5f);
            ImVec2 out(mid.x - center.x, mid.y - center.y);
            const float olen = std::sqrt(out.x * out.x + out.y * out.y);
            if (olen < 1e-3f)
                continue;
            out.x /= olen;
            out.y /= olen;
            const float gap = 7.f;
            ncirc[ncircN] = ImVec2(mid.x + out.x * (gap + handleR), mid.y + out.y * (gap + handleR));
            ncircDir[ncircN] = faceN[i];
            ++ncircN;
        }
    }

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    int hoverKind = -1;
    int hoverIdx = -1;
    glm::vec3 hoverDir(0.f);

    for (int i = 0; i < 8; ++i) {
        if (glm::dot(vtx[i], eye) < -0.2f)
            continue;
        const float dx = mouse.x - pv[i].x;
        const float dy = mouse.y - pv[i].y;
        if (dx * dx + dy * dy <= (handleR + 5.f) * (handleR + 5.f)) {
            hoverKind = 2;
            hoverIdx = i;
            hoverDir = glm::normalize(vtx[i]);
        }
    }
    if (hoverKind < 0) {
        for (int i = 0; i < ncircN; ++i) {
            const float dx = mouse.x - ncirc[i].x;
            const float dy = mouse.y - ncirc[i].y;
            if (dx * dx + dy * dy <= (handleR + 5.f) * (handleR + 5.f)) {
                hoverKind = 3;
                hoverIdx = i;
                hoverDir = ncircDir[i];
                break;
            }
        }
    }
    if (hoverKind < 0 && !ortho) {
        float best = 7.f;
        for (int i = 0; i < 12; ++i) {
            const int a = edgeV[i][0];
            const int b = edgeV[i][1];
            if (glm::dot(vtx[a] + vtx[b], eye) < 0.f)
                continue;
            const float d = dist_to_seg(mouse, pv[a], pv[b]);
            if (d < best) {
                best = d;
                hoverKind = 1;
                hoverIdx = i;
                hoverDir = glm::normalize(vtx[a] + vtx[b]);
            }
        }
    }
    if (hoverKind < 0) {
        for (int k = 5; k >= 0; --k) {
            const int i = faceOrder[k];
            if (faceF[i] < 0.02f)
                continue;
            ImVec2 q[4] = { pv[faceV[i][0]], pv[faceV[i][1]], pv[faceV[i][2]], pv[faceV[i][3]] };
            if (point_in_poly(mouse, q, 4)) {
                hoverKind = 0;
                hoverIdx = i;
                hoverDir = faceN[i];
                break;
            }
        }
    }

    if (ImGui::IsItemActivated() && hoverKind >= 0)
        begin_view_orbit(anim, orbitYaw, orbitPitch, hoverDir);
    if (hoverKind >= 0)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 edgeCol = IM_COL32(150, 150, 154, 255);
    const ImU32 ink = IM_COL32(80, 80, 84, 255);
    const ImU32 hot_fill = IM_COL32(186, 198, 214, 255);

    for (int k = 0; k < 6; ++k) {
        const int i = faceOrder[k];
        if (faceF[i] < 0.02f)
            continue;
        ImVec2 q[4] = { pv[faceV[i][0]], pv[faceV[i][1]], pv[faceV[i][2]], pv[faceV[i][3]] };
        const float cr = (q[1].x - q[0].x) * (q[2].y - q[0].y) - (q[1].y - q[0].y) * (q[2].x - q[0].x);
        if (cr < 0.f) {
            std::swap(q[1], q[3]);
        }
        const float shade = std::clamp(0.62f + 0.28f * faceF[i], 0.55f, 0.92f);
        const int g = int(shade * 255.f);
        const bool hot = (hoverKind == 0 && hoverIdx == i);
        const ImU32 fill = hot ? hot_fill : IM_COL32(g, g, g, 255);
        dl->AddConvexPolyFilled(q, 4, fill);
        dl->AddPolyline(q, 4, edgeCol, ImDrawFlags_Closed, 1.6f);
        if (faceF[i] > 0.22f) {
            const ImVec2 mid(
                (q[0].x + q[1].x + q[2].x + q[3].x) * 0.25f,
                (q[0].y + q[1].y + q[2].y + q[3].y) * 0.25f);
            const ImVec2 ts = ImGui::CalcTextSize(faceL[i]);
            dl->AddText(ImVec2(mid.x - ts.x * 0.5f, mid.y - ts.y * 0.5f), ink, faceL[i]);
        }
    }

    if (ortho) {
        for (int i = 0; i < 4; ++i) {
            const int vi = faceV[front][i];
            const bool hot = (hoverKind == 2 && hoverIdx == vi);
            dl->AddCircleFilled(pv[vi], handleR, hot ? IM_COL32(70, 110, 170, 255) : IM_COL32(236, 236, 238, 255), 14);
            dl->AddCircle(pv[vi], handleR, hot ? IM_COL32(70, 110, 170, 255) : edgeCol, 14, 1.3f);
        }
        for (int i = 0; i < ncircN; ++i) {
            const bool hot = (hoverKind == 3 && hoverIdx == i);
            dl->AddCircleFilled(ncirc[i], handleR, hot ? IM_COL32(70, 110, 170, 255) : IM_COL32(236, 236, 238, 255), 14);
            dl->AddCircle(ncirc[i], handleR, hot ? IM_COL32(70, 110, 170, 255) : edgeCol, 14, 1.3f);
        }
    }

    if (hoverKind == 1) {
        const int a = edgeV[hoverIdx][0];
        const int b = edgeV[hoverIdx][1];
        dl->AddLine(pv[a], pv[b], IM_COL32(70, 110, 170, 255), 3.2f);
    }
    if (hoverKind == 2 && !ortho)
        dl->AddCircleFilled(pv[hoverIdx], 5.f, IM_COL32(70, 110, 170, 255), 16);
}

inline void draw_mesh_orient_buttons(glm::quat& orient, MeshOrientAnim& anim)
{
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float label_w = ImGui::CalcTextSize("x:").x;
    const float btn_w = std::max(40.f, (ImGui::GetContentRegionAvail().x - label_w - gap * 2.f) * 0.5f);
    const ImVec2 btn(btn_w, 0.f);

    auto row = [&](const char* label, const char* plus, const char* minus, const glm::vec3& axis) {
        ImGui::PushID(label);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s:", label);
        ImGui::SameLine();
        if (ImGui::Button(plus, btn))
            begin_mesh_orient(anim, orient, axis, glm::half_pi<float>());
        ImGui::SameLine();
        if (ImGui::Button(minus, btn))
            begin_mesh_orient(anim, orient, axis, -glm::half_pi<float>());
        ImGui::PopID();
    };
    row("x", "+x", "-x", { 1.f, 0.f, 0.f });
    row("y", "+y", "-y", { 0.f, 1.f, 0.f });
    row("z", "+z", "-z", { 0.f, 0.f, 1.f });
}

inline void draw_preview_light_params(PreviewLight& L)
{
    HeaderIndent indent;
    ImGui::Checkbox("\xE5\x90\xAF\xE7\x94\xA8", &L.enabled);
    ImGui::BeginDisabled(!L.enabled);

    ImGui::RadioButton("RGB", &L.space, 0);
    ImGui::SameLine();
    ImGui::RadioButton("HSV", &L.space, 1);
    ImGui::SameLine();
    const glm::vec3 rgb = hsv_to_rgb(L.hue, L.sat, L.val);
    if (L.space == 0)
        ImGui::Text("%.2f, %.2f, %.2f", rgb.x, rgb.y, rgb.z);
    else
        ImGui::Text("%.0f, %.2f, %.2f", L.hue, L.sat, L.val);

    ImGui::SliderFloat("\xE8\x89\xB2\xE8\xB0\x83", &L.hue, 0.f, 360.f, "%.0f");
    ImGui::SliderFloat("\xE5\xBD\xA9\xE5\xBA\xA6", &L.sat, 0.f, 1.f, "%.2f");
    ImGui::SliderFloat("\xE5\x80\xBC", &L.val, 0.f, 1.f, "%.2f");
    ImGui::SliderFloat("\xE5\xBC\xBA\xE5\xBA\xA6", &L.intensity, 0.f, 10.f, "%.2f");

    ImGui::EndDisabled();
}

inline void draw_preview_light(int index, PreviewLight& L)
{
    ImGui::PushID(index);
    char title[32];
    std::snprintf(title, sizeof(title), "\xE5\x85\x89\xE7\xBA\xBF %d", index + 1);
    if (ImGui::CollapsingHeader(title))
        draw_preview_light_params(L);
    ImGui::PopID();
}

struct EnvironmentSettings {
    int mode = 0;
    glm::vec3 solidColor = glm::vec3(0.878f, 0.89f, 0.921f);
    HdrPreview hdr;
    GLuint envCube = 0;
    GLuint prefilterCube = 0;
    SH9 hdrSh{};
    std::string hdrError;
};

inline void draw_environment_panel(EnvironmentSettings& env)
{
    if (!ImGui::CollapsingHeader("\xE7\x8E\xAF\xE5\xA2\x83"))
        return;

    HeaderIndent indent;
    ImGui::RadioButton("Solid", &env.mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("HDR", &env.mode, 1);

    if (env.mode == 0) {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::ColorPicker3("##solid_ambient", &env.solidColor.x,
            ImGuiColorEditFlags_PickerHueWheel |
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_NoAlpha |
            ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoSmallPreview |
            ImGuiColorEditFlags_NoLabel);
        return;
    }

    const float side = std::min(ImGui::GetContentRegionAvail().x, 240.f);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + side, p0.y + side);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, p1, IM_COL32(48, 48, 52, 255), 4.f);
    if (env.hdr.tex != 0 && env.hdr.w > 0 && env.hdr.h > 0) {
        const float ia = float(env.hdr.w) / float(env.hdr.h);
        float dw = side;
        float dh = side;
        float ox = 0.f;
        float oy = 0.f;
        if (ia > 1.f) {
            dh = side / ia;
            oy = (side - dh) * 0.5f;
        } else {
            dw = side * ia;
            ox = (side - dw) * 0.5f;
        }
        dl->AddImage(ImTextureRef(ImTextureID(env.hdr.tex)),
            ImVec2(p0.x + ox, p0.y + oy),
            ImVec2(p0.x + ox + dw, p0.y + oy + dh));
    }
    dl->AddRect(p0, p1, IM_COL32(0, 0, 0, 60), 4.f);
    ImGui::Dummy(ImVec2(side, side));
    if (ImGui::Button("Change")) {
        std::wstring path;
        if (open_hdr_file_dialog(path))
            env.hdrError = hdr_environment_load(env.hdr, env.envCube, env.prefilterCube, env.hdrSh, path);
    }
    if (!env.hdrError.empty())
        ImGui::TextWrapped("%s", env.hdrError.c_str());
}

inline void draw_operate_pannel(int fb_w, int fb_h, Light& light, LightGizmo& gizmo,
    int& materialId, Material& material, Material1& material1, Material2& material2,
    Material3& material3, Material4& material4, bool& meshInspect, int& meshDrawMode, glm::quat& meshOrient, MeshOrientAnim& meshOrientAnim, const ObjMesh& mesh, EnvironmentSettings& env, Ssao& ssao, float& exposure,
    float& orbitYaw, float& orbitPitch, ViewOrbitAnim& viewAnim)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float panel_w = std::clamp(io.DisplaySize.x * 0.22f, 240.f, 380.f);
    const float panel_h = io.DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - panel_w, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panel_w, panel_h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("operate", nullptr, flags);

    tick_view_orbit(viewAnim, orbitYaw, orbitPitch, ImGui::GetIO().DeltaTime);
    tick_mesh_orient(meshOrientAnim, meshOrient, ImGui::GetIO().DeltaTime);

    if (ImGui::BeginTabBar("operate_tabs", ImGuiTabBarFlags_None)) {
        int tab = -1;
        if (ImGui::BeginTabItem("light")) {
            tab = 0;
            if (ImGui::CollapsingHeader("\xE5\x85\x89\xE6\xBA\x90\xE6\x97\x8B\xE8\xBD\xAC", ImGuiTreeNodeFlags_DefaultOpen)) {
                const float ring_r = std::clamp((panel_w - 48.f) * 0.28f, 48.f, 72.f);
                static PreviewLight sun_preview{ true, 0, 0.f, 0.f, 1.f, 1.f };
                light_yaw_ring("sun_yaw", gizmo, sun_preview, ring_r);
                light.dir = light_dir_from_gizmo(gizmo);
                ImGui::PushID("sun_rot_params");
                draw_preview_light_params(sun_preview);
                ImGui::PopID();
                light.color = hsv_to_rgb(sun_preview.hue, sun_preview.sat, sun_preview.val);
                light.intensity = sun_preview.intensity;
                light.enabled = sun_preview.enabled;
            }

            static PreviewLight preview_lights[5]{};
            for (int i = 0; i < 5; ++i)
                draw_preview_light(i, preview_lights[i]);

            draw_environment_panel(env);

            if (ImGui::CollapsingHeader("exposure")) {
                HeaderIndent indent;
                ImGui::SliderFloat("EV", &exposure, -4.f, 4.f, "%.1f");
            }

            if (ImGui::CollapsingHeader("SSAO")) {
                HeaderIndent indent;
                ImGui::Checkbox("enabled", &ssao.enabled);
                ImGui::BeginDisabled(!ssao.enabled);
                ImGui::SliderFloat("radius", &ssao.radius, 0.05f, 2.f, "%.2f");
                ImGui::SliderFloat("strength", &ssao.strength, 0.f, 1.f, "%.2f");
                ImGui::EndDisabled();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("material")) {
            tab = 1;
            ImGui::PushID(0);
            if (ImGui::CollapsingHeader("standard", ImGuiTreeNodeFlags_DefaultOpen)) {
                HeaderIndent indent;
                bool use0 = (materialId == 0);
                if (ImGui::Checkbox("\xE5\x90\xAF\xE7\x94\xA8", &use0) && use0)
                    materialId = 0;
                ImGui::BeginDisabled(materialId != 0);
                ImGui::ColorEdit3("albedo", &material.albedo.x, ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("metallic", &material.metallic, 0.f, 1.f);
                ImGui::SliderFloat("roughness", &material.roughness, 0.045f, 1.f);
                ImGui::EndDisabled();
            }
            ImGui::PopID();

            ImGui::PushID(1);
            if (ImGui::CollapsingHeader("clearcoat")) {
                HeaderIndent indent;
                bool use1 = (materialId == 1);
                if (ImGui::Checkbox("\xE5\x90\xAF\xE7\x94\xA8", &use1) && use1)
                    materialId = 1;
                ImGui::BeginDisabled(materialId != 1);
                ImGui::ColorEdit3("albedo", &material1.albedo.x, ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("metallic", &material1.metallic, 0.f, 1.f);
                ImGui::SliderFloat("roughness", &material1.roughness, 0.045f, 1.f);
                ImGui::SliderFloat("specular", &material1.specular, 0.f, 1.f);
                ImGui::SliderFloat("clearcoat##amount", &material1.clearcoat, 0.f, 1.f);
                ImGui::SliderFloat("clearcoatRoughness", &material1.clearcoatRoughness, 0.045f, 1.f);
                ImGui::EndDisabled();
            }
            ImGui::PopID();

            ImGui::PushID(2);
            if (ImGui::CollapsingHeader("transmission")) {
                HeaderIndent indent;
                bool use2 = (materialId == 2);
                if (ImGui::Checkbox("\xE5\x90\xAF\xE7\x94\xA8", &use2) && use2)
                    materialId = 2;
                ImGui::BeginDisabled(materialId != 2);
                ImGui::ColorEdit3("albedo", &material2.albedo.x, ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("roughness", &material2.roughness, 0.045f, 1.f);
                ImGui::SliderFloat("ior", &material2.ior, 1.01f, 2.5f);
                ImGui::SliderFloat("transmission##amount", &material2.transmission, 0.f, 1.f);
                ImGui::ColorEdit3("attenuationColor", &material2.attenuationColor.x, ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("attenuationDistance", &material2.attenuationDistance, 0.02f, 4.f);
                ImGui::EndDisabled();
            }
            ImGui::PopID();

            ImGui::PushID(3);
            if (ImGui::CollapsingHeader("subsurface")) {
                HeaderIndent indent;
                bool use3 = (materialId == 3);
                if (ImGui::Checkbox("\xE5\x90\xAF\xE7\x94\xA8", &use3) && use3)
                    materialId = 3;
                ImGui::BeginDisabled(materialId != 3);
                ImGui::ColorEdit3("albedo", &material3.albedo.x, ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("metallic", &material3.metallic, 0.f, 1.f);
                ImGui::SliderFloat("roughness", &material3.roughness, 0.045f, 1.f);
                ImGui::SliderFloat("specular", &material3.specular, 0.f, 1.f);
                ImGui::ColorEdit3("subsurfaceColor", &material3.subsurfaceColor.x, ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("opacity", &material3.opacity, 0.f, 1.f);
                ImGui::EndDisabled();
            }
            ImGui::PopID();

            ImGui::PushID(4);
            if (ImGui::CollapsingHeader("cloth")) {
                HeaderIndent indent;
                bool use4 = (materialId == 4);
                if (ImGui::Checkbox("\xE5\x90\xAF\xE7\x94\xA8", &use4) && use4)
                    materialId = 4;
                ImGui::BeginDisabled(materialId != 4);
                ImGui::ColorEdit3("albedo", &material4.albedo.x, ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("metallic", &material4.metallic, 0.f, 1.f);
                ImGui::SliderFloat("roughness", &material4.roughness, 0.045f, 1.f);
                ImGui::SliderFloat("specular", &material4.specular, 0.f, 1.f);
                ImGui::ColorEdit3("fuzzColor", &material4.fuzzColor.x, ImGuiColorEditFlags_Float);
                ImGui::SliderFloat("cloth##amount", &material4.cloth, 0.f, 1.f);
                ImGui::EndDisabled();
            }
            ImGui::PopID();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("mesh")) {
            tab = 2;
            if (ImGui::CollapsingHeader("\xE7\xBB\x9F\xE8\xAE\xA1\xE4\xBF\xA1\xE6\x81\xAF", ImGuiTreeNodeFlags_DefaultOpen)) {
                HeaderIndent indent;
                ImGui::Text("vertices  %zu", mesh.positions.size());
                ImGui::Text("triangles %zu", mesh.indices.size() / 3);
            }
            if (ImGui::CollapsingHeader("\xE8\xA7\x86\xE7\x82\xB9", ImGuiTreeNodeFlags_DefaultOpen))
                draw_mesh_view_cube(orbitYaw, orbitPitch, viewAnim);
            if (ImGui::CollapsingHeader("\xE6\x97\x8B\xE8\xBD\xAC", ImGuiTreeNodeFlags_DefaultOpen)) {
                HeaderIndent indent;
                draw_mesh_orient_buttons(meshOrient, meshOrientAnim);
            }
            if (ImGui::CollapsingHeader("\xE7\xBD\x91\xE6\xA0\xBC")) {
                HeaderIndent indent;
                const char* drawModes[] = { "face", "point", "wireframe" };
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("draw type");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::Combo("##mesh_draw_mode", &meshDrawMode, drawModes, IM_ARRAYSIZE(drawModes));
            }
            ImGui::EndTabItem();
        }

        static int lastTab = 0;
        if (tab == 2 && lastTab != 2)
            meshInspect = true;
        else if (tab != 2 && lastTab == 2)
            meshInspect = false;
        if (tab >= 0)
            lastTab = tab;

        ImGui::EndTabBar();
    }

    ImGui::End();
    (void)fb_w;
    (void)fb_h;
}
