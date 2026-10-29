#pragma once

#include <string>

const std::string vShaderCode = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    layout(location = 2) in vec2 aUV;

    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;

    out vec3 vWorldPos;
    out vec3 vNormal;

    void main() {
        vWorldPos = aPos;
        vNormal = aNormal;
        gl_Position = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
    }
)";

const std::string fShaderCode = R"(
    #version 460 core

    in vec3 vWorldPos;
    in vec3 vNormal;

    uniform vec3 cameraPos;

    layout(location = 0) out vec4 fragColor;

    void main() {
        vec3 N = normalize(vNormal);
        vec3 V = normalize(cameraPos - vWorldPos);

        float facing = abs(dot(N, V));

        vec3 albedo = vec3(0.75, 0.75, 0.78);
        vec3 color = albedo * mix(0.25, 1.0, facing);

        fragColor = vec4(color, 1.0);
    }
)";