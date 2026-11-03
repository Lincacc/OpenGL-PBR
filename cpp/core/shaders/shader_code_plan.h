#pragma once

#include <string>

const std::string vGroundShaderCode = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;

    uniform mat4 viewMatrix;
    uniform mat4 projectionMatrix;
    uniform float floorY;

    out vec3 vWorldPos;

    void main() {
        vec3 wp = vec3(aPos.x, floorY, aPos.z);
        vWorldPos = wp;
        gl_Position = projectionMatrix * viewMatrix * vec4(wp, 1.0);
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
    uniform sampler2DShadow shadowMap;
    uniform mat4 lightViewProj;
    uniform int shadowEnabled;

    layout(location = 0) out vec4 fragDirect;
    layout(location = 1) out vec4 fragDiffuseIbl;

    float sunShadow(vec3 worldPos) {
        if (shadowEnabled == 0)
            return 1.0;
        vec4 lp = lightViewProj * vec4(worldPos, 1.0);
        vec3 uvd = lp.xyz / lp.w * 0.5 + 0.5;
        if (uvd.x < 0.0 || uvd.x > 1.0 || uvd.y < 0.0 || uvd.y > 1.0 || uvd.z > 1.0)
            return 1.0;
        uvd.z -= 0.002;
        vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
        float sum = 0.0;
        for (int y = -2; y <= 2; ++y) {
            for (int x = -2; x <= 2; ++x)
                sum += texture(shadowMap, uvd + vec3(vec2(float(x), float(y)) * texel, 0.0));
        }
        return sum / 25.0;
    }

    void main() {
        float rFade = length(vWorldPos.xz);
        float fade = 1.0 - smoothstep(fadeRadius * 0.55, fadeRadius, rFade);
        if (fade < 1e-3)
            discard;

        vec2 d = vWorldPos.xz - aoCenter;
        float r = length(d);
        float contact = exp(-(r * r) / max(aoRadius * aoRadius, 1e-6));
        float sh = sunShadow(vWorldPos);
        float hint = (1.0 - sh) * 0.25 * (1.0 - contact);
        vec3 lit = ambientColor * (1.0 - aoStrength * contact - hint);

        fragDirect = vec4(ambientColor * (1.0 - fade), 1.0);
        fragDiffuseIbl = vec4(lit * fade, 1.0);
    }
)";
