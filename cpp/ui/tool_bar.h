#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commdlg.h>

#include "core/obj_loader.h"
#include "core/render/actor.h"
#include "ui/operate_pannel.h"

#include <imgui.h>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <exception>
#include <filesystem>
#include <string>

#pragma comment(lib, "comdlg32.lib")

inline bool open_obj_file_dialog(std::wstring& out_path) {
    std::wstring buf(1024, L'\0');
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = L"Wavefront OBJ (*.obj)\0*.obj\0";
    ofn.lpstrFile = buf.data();
    ofn.nMaxFile = static_cast<DWORD>(buf.size());
    ofn.lpstrTitle = L"Open";
    ofn.lpstrDefExt = L"obj";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn))
        return false;
    out_path = buf.c_str();
    return !out_path.empty();
}

inline bool save_obj_file_dialog(std::wstring& out_path) {
    std::wstring buf(1024, L'\0');
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = L"Wavefront OBJ (*.obj)\0*.obj\0";
    ofn.lpstrFile = buf.data();
    ofn.nMaxFile = static_cast<DWORD>(buf.size());
    ofn.lpstrTitle = L"Save data";
    ofn.lpstrDefExt = L"obj";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (!GetSaveFileNameW(&ofn))
        return false;
    out_path = buf.c_str();
    return !out_path.empty();
}

inline ObjMesh bake_oriented_mesh(const ObjMesh& src, const glm::mat3& R) {
    ObjMesh out = src;
    for (auto& p : out.positions)
        p = R * p;
    for (auto& n : out.normals) {
        n = R * n;
        const float len = glm::length(n);
        n = (len > 1e-8f) ? (n / len) : glm::vec3(0.f, 1.f, 0.f);
    }
    return out;
}

inline void draw_tool_bar(Actor& actor, MeshOrientAnim& anim) {
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open")) {
            std::wstring path;
            if (open_obj_file_dialog(path)) {
                try {
                    actor.set_mesh(load_obj(path));
                    actor.meshOrient = glm::quat(1.f, 0.f, 0.f, 0.f);
                    anim.t = 1.f;
                    anim.from = actor.meshOrient;
                    anim.to = actor.meshOrient;
                } catch (const std::exception& e) {
                    MessageBoxA(GetActiveWindow(), e.what(), "Open", MB_OK | MB_ICONERROR);
                }
            }
        }
        if (ImGui::MenuItem("Save data")) {
            std::wstring path;
            if (save_obj_file_dialog(path)) {
                try {
                    save_obj(path, bake_oriented_mesh(actor.mesh, actor.mesh_rotation()));
                } catch (const std::exception& e) {
                    MessageBoxA(GetActiveWindow(), e.what(), "Save data", MB_OK | MB_ICONERROR);
                }
            }
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}
