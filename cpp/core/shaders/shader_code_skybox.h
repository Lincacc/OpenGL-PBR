#pragma once

#include <string>

const std::string vSkyboxShaderCode = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;

    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;

    out vec3 vLocalPos;

    void main() {
        vLocalPos = aPos;
        vec4 clip = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
        gl_Position = clip.xyww;
    }
)";

const std::string fSkyboxShaderCode = R"(
    #version 460 core

    in vec3 vLocalPos;

    uniform samplerCube envMap;

    layout(location = 0) out vec4 fragColor;

    void main() {
        fragColor = vec4(texture(envMap, normalize(vLocalPos)).rgb, 1.0);
    }
)";

const std::string vEquirectToCubeShaderCode = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;

    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;

    out vec3 vLocalPos;

    void main() {
        vLocalPos = aPos;
        gl_Position = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
    }
)";

const std::string fEquirectToCubeShaderCode = R"(
    #version 460 core

    in vec3 vLocalPos;

    uniform sampler2D equirectMap;

    layout(location = 0) out vec4 fragColor;

    const vec2 invAtan = vec2(0.15915494309, 0.31830988618);

    vec2 sampleEquirect(vec3 v) {
        vec3 n = normalize(v);
        vec2 uv = vec2(atan(n.z, n.x), asin(clamp(n.y, -1.0, 1.0)));
        uv *= invAtan;
        uv += 0.5;
        uv.y = 1.0 - uv.y;
        return uv;
    }

    void main() {
        fragColor = vec4(texture(equirectMap, sampleEquirect(vLocalPos)).rgb, 1.0);
    }
)";
