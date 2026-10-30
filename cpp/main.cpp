#include "core/core.h"
#include "core/actor.h"
#include "core/ground.h"
#include "core/skybox.h"
#include "core/light.h"
#include "core/sh.h"
#include "core/shaders/shader_code_0.h"
#include "core/materials/material_0.h"
#include "core/camera.h"
#include "ui/operate_pannel.h"

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

int width = 1280;
int height = 720;

LightGizmo sun_gizmo;
Light sun;
EnvironmentSettings environment;
glm::vec3 ambientColor = environment.solidColor;

Camera camera;
float orbitYaw = 0.f;
float orbitPitch = 0.4f;
float orbitRadius = 4.f;
bool orbitDragging = false;
double orbitLastX = 0.0;
double orbitLastY = 0.0;

static void apply_orbit() {
    float cp = std::cos(orbitPitch);
    camera.position = {
        orbitRadius * cp * std::cos(orbitYaw),
        orbitRadius * std::sin(orbitPitch),
        orbitRadius * cp * std::sin(orbitYaw)
    };
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }
    orbitDragging = (action == GLFW_PRESS);
    glfwGetCursorPos(window, &orbitLastX, &orbitLastY);
}

static void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);
    if (ImGui::GetIO().WantCaptureMouse || !orbitDragging) {
        return;
    }

    orbitYaw += float(x - orbitLastX) * 0.005f;
    orbitPitch += float(orbitLastY - y) * 0.005f;
    orbitPitch = std::clamp(orbitPitch, -1.55f, 1.55f);

    orbitLastX = x;
    orbitLastY = y;
    apply_orbit();
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    orbitRadius *= std::pow(0.9f, float(yoffset));
    orbitRadius = std::clamp(orbitRadius, 0.5f, 20.f);
    apply_orbit();
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);  // 8x MSAA

    GLFWwindow* window = glfwCreateWindow(width, height, "Weather System", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    if (!gladLoadGL()) {
        throw std::runtime_error("load gl error!");
    }

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    apply_orbit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();
    {
        const ImVec4 bg(0.937f, 0.956f, 0.976f, 1.f);
        ImGuiStyle& st = ImGui::GetStyle();
        st.Colors[ImGuiCol_WindowBg] = bg;
        st.Colors[ImGuiCol_ChildBg] = bg;
        st.Colors[ImGuiCol_TitleBg] = bg;
        st.Colors[ImGuiCol_TitleBgActive] = bg;
        st.Colors[ImGuiCol_TitleBgCollapsed] = bg;
        st.Colors[ImGuiCol_MenuBarBg] = bg;
        st.Colors[ImGuiCol_Header] = bg;
        st.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.729f, 0.741f, 0.764f, 1.f);
        st.Colors[ImGuiCol_HeaderActive] = ImVec4(0.729f, 0.741f, 0.764f, 1.f);
        st.Colors[ImGuiCol_Tab] = bg;
        st.Colors[ImGuiCol_TabHovered] = ImVec4(0.729f, 0.741f, 0.764f, 1.f);
        st.Colors[ImGuiCol_TabSelected] = ImVec4(0.729f, 0.741f, 0.764f, 1.f);
        st.Colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.729f, 0.741f, 0.764f, 1.f);
        st.Colors[ImGuiCol_TabDimmed] = ImVec4(0.729f, 0.741f, 0.764f, 1.f);
        st.Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.729f, 0.741f, 0.764f, 1.f);
        st.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.729f, 0.741f, 0.764f, 1.f);
        st.Colors[ImGuiCol_Border] = ImVec4(0.f, 0.f, 0.f, 0.f);
    }
    {
        ImGuiIO& imgui_io = ImGui::GetIO();
        ImFontConfig font_cfg;
        font_cfg.OversampleH = 2;
        font_cfg.FontNo = 0;
        const char* font_path = "C:\\Windows\\Fonts\\msyh.ttc";
        if (!imgui_io.Fonts->AddFontFromFileTTF(font_path, 18.f, &font_cfg,
            imgui_io.Fonts->GetGlyphRangesChineseSimplifiedCommon()))
            imgui_io.Fonts->AddFontDefault();
    }
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 460");

    sun.dir = glm::normalize(glm::vec3(1.f));
    sun.color = glm::vec3(1.f);
    sun.intensity = 1.f;
    sun.enabled = true;
    sun_gizmo = light_gizmo_from_dir(sun.dir);

    Actor actor;
    actor.shader.compile(vShaderCode, fShaderCode);
    actor.mesh = load_obj("cpp/resource/bunny.obj");
    actor.material.albedo = glm::vec3(1, 0, 1);
    actor.material.matParams.r = 0;
    actor.material.matParams.g = 0.1;
    actor.material.matParams.b = 0;
    actor.init();

    glm::vec3 bmin(1e9f), bmax(-1e9f);
    for (const auto& pos : actor.mesh.positions) {
        bmin = glm::min(bmin, pos);
        bmax = glm::max(bmax, pos);
    }
    const float floorY = bmin.y - 0.002f;
    const glm::vec2 aoCenter = (glm::vec2(bmin.x, bmin.z) + glm::vec2(bmax.x, bmax.z)) * 0.5f;
    const float aoRadius = 0.55f * glm::length(glm::vec2(bmax.x - bmin.x, bmax.z - bmin.z));

    Ground ground;
    ground.init(floorY, 4.f);

    Skybox skybox;
    skybox.init();
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    SH9 shAmbient = sh_irradiance_from_sky_ground(environment.solidColor, environment.solidColor * 0.25f);
    glm::vec3 shSolidColor = environment.solidColor;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    while (!glfwWindowShouldClose(window)) {
        if (environment.solidColor != shSolidColor) {
            shAmbient = sh_irradiance_from_sky_ground(environment.solidColor, environment.solidColor * 0.25f);
            shSolidColor = environment.solidColor;
        }
        ambientColor = environment.solidColor;
        glfwPollEvents();
        glfwGetFramebufferSize(window, &width, &height);

        camera.aspect = float(width) / float(height);
        const auto& v = camera.view();
        const auto& p = camera.project();
        
        glViewport(0, 0, width, height);
        glClearColor(ambientColor.r, ambientColor.g, ambientColor.b, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ground.draw(v, p, ambientColor, aoCenter, aoRadius);

        actor.shader.bind();
        actor.shader.setMatrix4("viewMatrix", v);
        actor.shader.setMatrix4("projectionMatrix", p);
        actor.shader.setVec3f("cameraPos", camera.position);

        actor.shader.setVec3f("lightDir", sun.dir);
        actor.shader.setVec3f("lightColor",
            sun.enabled ? sun.color * sun.intensity : glm::vec3(0.f));
        actor.shader.setVec3Array("shIrradiance", shAmbient.c, 9);

        actor.draw();

        if (environment.mode == 1 && environment.envCube != 0)
            skybox.draw(v, p, environment.envCube);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_operate_pannel(width, height, sun, sun_gizmo, actor.material, actor.mesh, environment);

#pragma region memory usage
        auto usage = host_usage();
        char text[128];
        std::snprintf(text, sizeof(text), "fps %.1f\ncpu %.0f%%\ngpu %.0f%%",
            ImGui::GetIO().Framerate, usage.cpu, usage.gpu);
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        draw_list->AddText(ImVec2(9.f, 9.f), IM_COL32(0, 0, 0, 180), text);
        draw_list->AddText(ImVec2(8.f, 8.f), IM_COL32(255, 255, 255, 255), text);
#pragma endregion

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    hdr_environment_destroy(environment.hdr, environment.envCube);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}