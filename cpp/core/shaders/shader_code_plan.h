#pragma once

#include <string>

const std::string vGroundShaderCode = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;

    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;

    out vec3 vWorldPos;

    void main() {
        vWorldPos = aPos;
        gl_Position = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
    }
)";

const std::string fGroundShaderCode = R"(
    #version 460 core

    in vec3 vWorldPos;

    uniform vec3 ambientColor;
    uniform vec2 aoCenter;
    uniform float aoRadius;
    uniform float aoStrength;
    uniform float fadeRadius;

    layout(location = 0) out vec4 fragColor;

    void main() {
        vec2 d = vWorldPos.xz - aoCenter;
        float r = length(d);

        float ao = exp(-(r * r) / max(aoRadius * aoRadius, 1e-6));
        vec3 color = ambientColor * (1.0 - aoStrength * ao);

        float fade = 1.0 - smoothstep(fadeRadius * 0.55, fadeRadius, r);
        if (fade < 1e-3)
            discard;

        fragColor = vec4(color, fade);
    }
)";
