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
    uniform float exposure;

    layout(location = 0) out vec4 fragColor;

    const mat3 kAcesInput = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
    );

    const mat3 kAcesOutput = mat3(
         1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
    );

    vec3 rrtAndOdtFit(vec3 v) {
        vec3 a = v * (v + 0.0245786) - 0.000090537;
        vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
        return a / b;
    }

    vec3 acesFitted(vec3 color) {
        color = kAcesInput * color;
        color = rrtAndOdtFit(color);
        color = kAcesOutput * color;
        return clamp(color, 0.0, 1.0);
    }

    void main() {
        float ao = 1.0;
        if (ssaoEnabled != 0)
            ao = mix(1.0, texture(ssaoMap, vUV).r, clamp(ssaoStrength, 0.0, 1.0));

        vec3 color = texture(directMap, vUV).rgb + texture(iblMap, vUV).rgb * ao;
        color *= exp2(exposure);
        color = acesFitted(color);
        color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
        fragColor = vec4(color, 1.0);
    }
)";
