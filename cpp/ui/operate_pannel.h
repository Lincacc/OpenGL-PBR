#pragma once

#include "core/render/hdr_preview.h"
#include "core/render/ssao.h"
#include "core/light.h"
#include "core/materials/material_0.h"
#include "core/materials/material_1.h"
#include "core/obj_loader.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

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

inline void draw_preview_light_params(PreviewLight& L)
{
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
    int& materialId, Material& material, Material1& material1, const ObjMesh& mesh,
    EnvironmentSettings& env, Ssao& ssao, float& exposure)
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

    if (ImGui::BeginTabBar("operate_tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("light")) {
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
                ImGui::SliderFloat("EV", &exposure, -4.f, 4.f, "%.1f");
            }

            if (ImGui::CollapsingHeader("SSAO")) {
                ImGui::Checkbox("enabled", &ssao.enabled);
                ImGui::BeginDisabled(!ssao.enabled);
                ImGui::SliderFloat("radius", &ssao.radius, 0.05f, 2.f, "%.2f");
                ImGui::SliderFloat("strength", &ssao.strength, 0.f, 1.f, "%.2f");
                ImGui::EndDisabled();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("material")) {
            ImGui::PushID(0);
            bool use0 = (materialId == 0);
            if (ImGui::Checkbox("standard", &use0) && use0)
                materialId = 0;
            ImGui::ColorEdit3("albedo", &material.albedo.x, ImGuiColorEditFlags_Float);
            ImGui::SliderFloat("metallic", &material.metallic, 0.f, 1.f);
            ImGui::SliderFloat("roughness", &material.roughness, 0.045f, 1.f);
            ImGui::PopID();

            ImGui::PushID(1);
            bool use1 = (materialId == 1);
            if (ImGui::Checkbox("clearcoat##select", &use1) && use1)
                materialId = 1;
            ImGui::ColorEdit3("albedo", &material1.albedo.x, ImGuiColorEditFlags_Float);
            ImGui::SliderFloat("metallic", &material1.metallic, 0.f, 1.f);
            ImGui::SliderFloat("roughness", &material1.roughness, 0.045f, 1.f);
            ImGui::SliderFloat("clearcoat##amount", &material1.clearcoat, 0.f, 1.f);
            ImGui::SliderFloat("clearcoatRoughness", &material1.clearcoatRoughness, 0.045f, 1.f);
            ImGui::PopID();
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
