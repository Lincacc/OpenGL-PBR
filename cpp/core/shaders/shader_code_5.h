#pragma once

#include <string>

const std::string vShaderCode5 = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    layout(location = 2) in vec2 aUV;

    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;
    uniform mat3 meshRotation;

    out vec3 vWorldPos;
    out vec3 vNormal;

    void main() {
        vec3 worldPos = meshRotation * aPos;
        vWorldPos = worldPos;
        vNormal = meshRotation * aNormal;
        gl_Position = projectionMatrix * viewMatrix * vec4(worldPos, 1.0);
    }
)";

const std::string vShaderCode5Point = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;

    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;
    uniform mat3 meshRotation;
    uniform float pointRadius;
    uniform float viewportHeight;

    out vec3 vWorldPos;
    out vec3 vNormal;

    void main() {
        vec3 worldPos = meshRotation * aPos;
        vWorldPos = worldPos;
        vNormal = meshRotation * aNormal;
        vec4 viewPos = viewMatrix * vec4(worldPos, 1.0);
        gl_Position = projectionMatrix * viewPos;
        float pix = pointRadius * projectionMatrix[1][1] * viewportHeight / max(abs(viewPos.z), 1e-4);
        gl_PointSize = clamp(pix, 2.0, 64.0);
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

const std::string fShaderCode5Point = R"(
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
        vec2 pc = gl_PointCoord * 2.0 - 1.0;
        if (dot(pc, pc) > 1.0)
            discard;
        vec3 N = normalize(vNormal);
        vec3 V = normalize(cameraPos - vWorldPos);
        float ndv = abs(dot(N, V));
        float w = pow(clamp(ndv, 0.0, 1.0), max(power, 1e-3));
        fragDirect = vec4(mix(rim, albedo, w), 1.0);
        fragDiffuseIbl = vec4(0.0);
    }
)";
