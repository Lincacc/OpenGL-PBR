#pragma once

#include "core/light.h"
#include "core/materials/material_0.h"
#include "core/obj_loader.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

struct LightGizmo {
    float yaw = 0.785398f;
    float pitch = 0.615480f;
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

inline ImVec2 project_sphere(const glm::vec3& p, ImVec2 center, float R,
    const glm::vec3& right, const glm::vec3& up)
{
    return ImVec2(center.x + glm::dot(p, right) * R,
        center.y - glm::dot(p, up) * R);
}

inline void draw_great_circle(ImDrawList* dl, ImVec2 center, float R,
    const glm::vec3& axis_a, const glm::vec3& axis_b,
    const glm::vec3& right, const glm::vec3& up,
    ImU32 col, float thickness)
{
    ImVec2 prev{};
    bool has_prev = false;
    for (int i = 0; i <= 96; ++i) {
        const float t = float(i) / 96.f * glm::two_pi<float>();
        const glm::vec3 p = axis_a * std::cos(t) + axis_b * std::sin(t);
        const ImVec2 q = project_sphere(p, center, R, right, up);
        if (has_prev)
            dl->AddLine(prev, q, col, thickness);
        prev = q;
        has_prev = true;
    }
}

inline float dist_point_seg(ImVec2 p, ImVec2 a, ImVec2 b) {
    const ImVec2 ab(b.x - a.x, b.y - a.y);
    const float len2 = ab.x * ab.x + ab.y * ab.y;
    if (len2 < 1e-8f) {
        const float dx = p.x - a.x;
        const float dy = p.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2, 0.f, 1.f);
    const float dx = p.x - (a.x + ab.x * t);
    const float dy = p.y - (a.y + ab.y * t);
    return std::sqrt(dx * dx + dy * dy);
}

inline float dist_to_circle(ImVec2 mouse, ImVec2 center, float R,
    const glm::vec3& axis_a, const glm::vec3& axis_b,
    const glm::vec3& right, const glm::vec3& up)
{
    float best = 1e9f;
    ImVec2 prev{};
    bool has_prev = false;
    for (int i = 0; i <= 96; ++i) {
        const float t = float(i) / 96.f * glm::two_pi<float>();
        const glm::vec3 p = axis_a * std::cos(t) + axis_b * std::sin(t);
        const ImVec2 q = project_sphere(p, center, R, right, up);
        if (has_prev)
            best = std::min(best, dist_point_seg(mouse, prev, q));
        prev = q;
        has_prev = true;
    }
    return best;
}

inline void light_dir_sphere(const char* id, LightGizmo& g, Light& light, float radius)
{
    ImGui::PushID(id);
    const float box = radius * 2.f + 28.f;
    const float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.f, (avail - box) * 0.5f));

    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("sphere", ImVec2(box, box));
    const ImVec2 center(cursor.x + box * 0.5f, cursor.y + box * 0.5f);

    const glm::vec3 eye = glm::normalize(glm::vec3(-0.55f, 0.42f, 1.f));
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), eye));
    glm::vec3 up = glm::normalize(glm::cross(eye, right));

    const glm::vec3 y_a(1.f, 0.f, 0.f);
    const glm::vec3 y_b(0.f, 0.f, 1.f);
    const glm::vec3 horiz(std::sin(g.yaw), 0.f, std::cos(g.yaw));
    const glm::vec3 x_a = horiz;
    const glm::vec3 x_b(0.f, 1.f, 0.f);

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const float core_r = radius * 0.42f;
    const float md_x = mouse.x - center.x;
    const float md_y = mouse.y - center.y;
    const bool on_core = (md_x * md_x + md_y * md_y) <= core_r * core_r;

    const float dist_x = dist_to_circle(mouse, center, radius, x_a, x_b, right, up);
    const float dist_y = dist_to_circle(mouse, center, radius, y_a, y_b, right, up);
    const float pick_px = 8.f;
    int hover_ring = -1;
    if (dist_x < pick_px || dist_y < pick_px)
        hover_ring = (dist_x < dist_y) ? 0 : 1;

    static int drag_ring = -1;
    if (ImGui::IsItemActivated()) {
        if (hover_ring >= 0)
            drag_ring = hover_ring;
        else if (on_core)
            ImGui::OpenPopup("light_color");
    }
    if (!ImGui::IsItemActive())
        drag_ring = -1;

    if (ImGui::IsItemActive() && drag_ring >= 0) {
        const ImVec2 d = ImGui::GetIO().MouseDelta;
        const float sens = 2.f / radius;
        if (drag_ring == 1)
            g.yaw += d.x * sens;
        else
            g.pitch = std::clamp(g.pitch - d.y * sens, -1.55f, 1.55f);
    }

    const int lit_ring = drag_ring >= 0 ? drag_ring : hover_ring;
    const glm::vec3 col = glm::clamp(light.color, 0.f, 1.f);
    const ImU32 ball = color_u32(col, 255);
    const ImU32 ring_idle = IM_COL32(70, 150, 255, 255);
    const ImU32 ring_hot = IM_COL32(255, 210, 40, 255);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(center, core_r, ball, 32);
    dl->AddCircle(center, core_r, IM_COL32(255, 255, 255, 80), 32, 1.2f);

    const bool hi_y = lit_ring == 1;
    const bool hi_x = lit_ring == 0;
    const ImU32 col_y = hi_y ? ring_hot : ring_idle;
    const ImU32 col_x = hi_x ? ring_hot : ring_idle;
    draw_great_circle(dl, center, radius, y_a, y_b, right, up,
        col_y, hi_y ? 3.2f : 2.f);
    draw_great_circle(dl, center, radius, x_a, x_b, right, up,
        col_x, hi_x ? 3.2f : 2.f);

    const glm::vec3 dir = light_dir_from_gizmo(g);
    draw_sun_icon(dl, project_sphere(dir, center, radius, right, up), 5.f, ball);

    if (ImGui::BeginPopup("light_color")) {
        ImGui::ColorPicker3("##light_color", &light.color.x,
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoSidePreview);
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

inline void draw_operate_pannel(int fb_w, int fb_h, Light& light, LightGizmo& gizmo,
    Material& material, const ObjMesh& mesh)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float panel_w = std::clamp(io.DisplaySize.x * 0.22f, 240.f, 380.f);
    const float panel_h = io.DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - panel_w, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panel_w, panel_h), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("operate", nullptr, flags);

    if (ImGui::BeginTabBar("operate_tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("light")) {
            const float sphere_r = std::clamp((panel_w - 48.f) * 0.28f, 48.f, 72.f);
            ImGui::TextUnformatted("direction");
            light_dir_sphere("sun_sphere", gizmo, light, sphere_r);
            light.dir = light_dir_from_gizmo(gizmo);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("material")) {
            ImGui::ColorEdit3("albedo", &material.albedo.x, ImGuiColorEditFlags_Float);
            ImGui::SliderFloat("metallic", &material.matParams.x, 0.f, 1.f);
            ImGui::SliderFloat("roughness", &material.matParams.y, 0.045f, 1.f);
            ImGui::SliderFloat("ao", &material.matParams.z, 0.f, 1.f);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("mesh")) {
            ImGui::Text("vertices  %zu", mesh.positions.size());
            ImGui::Text("triangles %zu", mesh.indices.size() / 3);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
    (void)fb_w;
    (void)fb_h;
}