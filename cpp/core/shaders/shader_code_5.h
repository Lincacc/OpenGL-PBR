#pragma once

#include <string>

const std::string vShaderCode5 = R"(
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

const std::string fShaderCode5 = R"(
    #version 460 core

    in vec3 vWorldPos;
    in vec3 vNormal;

    uniform vec3 cameraPos;
    uniform vec3 albedo;
    uniform vec3 rim;
    uniform float power;

    layout(location = 0) out vec4 fragDirect;
    layout(location = 1) out vec4 fragDiffuseIbl;

    void main() {
        vec3 N = normalize(vNormal);
        vec3 V = normalize(cameraPos - vWorldPos);
        float ndv = abs(dot(N, V));
        float w = pow(clamp(ndv, 0.0, 1.0), max(power, 1e-3));
        fragDirect = vec4(mix(rim, albedo, w), 1.0);
        fragDiffuseIbl = vec4(0.0);
    }
)";
