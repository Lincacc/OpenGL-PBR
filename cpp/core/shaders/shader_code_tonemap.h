#pragma once

#include <string>

const std::string vTonemapShaderCode = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;

    out vec2 vUV;

    void main() {
        vUV = aPos.xy * 0.5 + 0.5;
        gl_Position = vec4(aPos, 1.0);
    }
)";

const std::string fTonemapShaderCode = R"(
    #version 460 core

    in vec2 vUV;

    uniform sampler2D directMap;
    uniform sampler2D iblMap;
    uniform sampler2D ssaoMap;
    uniform int ssaoEnabled;
    uniform float ssaoStrength;

    layout(location = 0) out vec4 fragColor;

    void main() {
        float ao = 1.0;
        if (ssaoEnabled != 0)
            ao = mix(1.0, texture(ssaoMap, vUV).r, clamp(ssaoStrength, 0.0, 1.0));

        vec3 color = texture(directMap, vUV).rgb + texture(iblMap, vUV).rgb * ao;
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));
        fragColor = vec4(color, 1.0);
    }
)";
