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

    uniform sampler2D hdrMap;

    layout(location = 0) out vec4 fragColor;

    void main() {
        vec3 color = texture(hdrMap, vUV).rgb;
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));
        fragColor = vec4(color, 1.0);
    }
)";
