#pragma once

#include <string>

const std::string vIblLutShaderCode = R"(
    #version 460 core

    out vec2 vUV;

    void main() {
        vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
        vUV = p;
        gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
    }
)";

const std::string fIblLutShaderCode = R"(
    #version 460 core

    in vec2 vUV;

    uniform int sampleCount;

    layout(location = 0) out vec2 fragColor;

    const float PI = 3.14159265359;

    float radicalInverseVdC(uint bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10;
    }

    vec2 hammersley(uint i, uint n) {
        return vec2(float(i) / float(n), radicalInverseVdC(i));
    }

    vec3 importanceSampleGGX(vec2 xi, vec3 n, float roughness) {
        float a = roughness * roughness;
        float phi = 2.0 * PI * xi.x;
        float cos_t = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
        float sin_t = sqrt(max(1.0 - cos_t * cos_t, 0.0));
        vec3 h = vec3(cos(phi) * sin_t, sin(phi) * sin_t, cos_t);
        vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
        vec3 tangent = normalize(cross(up, n));
        vec3 bitangent = cross(n, tangent);
        return normalize(tangent * h.x + bitangent * h.y + n * h.z);
    }

    float geometrySchlickGGX(float nDotV, float roughness) {
        float k = (roughness * roughness) / 2.0;
        return nDotV / (nDotV * (1.0 - k) + k);
    }

    void main() {
        float nDotV = max(vUV.x, 1e-4);
        float roughness = clamp(vUV.y, 0.0, 1.0);

        vec3 n = vec3(0.0, 0.0, 1.0);
        vec3 v = vec3(sqrt(max(1.0 - nDotV * nDotV, 0.0)), 0.0, nDotV);

        float scale = 0.0;
        float bias = 0.0;
        uint count = uint(max(sampleCount, 1));
        for (uint i = 0u; i < count; ++i) {
            vec3 h = importanceSampleGGX(hammersley(i, count), n, roughness);
            vec3 l = normalize(2.0 * dot(v, h) * h - v);
            float nDotL = max(l.z, 0.0);
            float nDotH = max(h.z, 0.0);
            float vDotH = max(dot(v, h), 0.0);
            if (nDotL <= 0.0)
                continue;
            float g = geometrySchlickGGX(nDotV, roughness) * geometrySchlickGGX(nDotL, roughness);
            float gVis = (g * vDotH) / max(nDotH * nDotV, 1e-5);
            float fc = pow(1.0 - vDotH, 5.0);
            scale += (1.0 - fc) * gVis;
            bias += fc * gVis;
        }
        float inv = 1.0 / float(count);
        fragColor = vec2(scale * inv, bias * inv);
    }
)";

const std::string fIblSkyGroundShaderCode = R"(
    #version 460 core

    in vec3 vLocalPos;

    uniform vec3 skyColor;
    uniform vec3 groundColor;

    layout(location = 0) out vec4 fragColor;

    void main() {
        vec3 n = normalize(vLocalPos);
        fragColor = vec4(n.y >= 0.0 ? skyColor : groundColor, 1.0);
    }
)";

const std::string fIblPrefilterShaderCode = R"(
    #version 460 core

    in vec3 vLocalPos;

    uniform samplerCube envMap;
    uniform float roughness;
    uniform float envResolution;
    uniform int sampleCount;

    layout(location = 0) out vec4 fragColor;

    const float PI = 3.14159265359;

    float radicalInverseVdC(uint bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return float(bits) * 2.3283064365386963e-10;
    }

    vec2 hammersley(uint i, uint n) {
        return vec2(float(i) / float(n), radicalInverseVdC(i));
    }

    vec3 importanceSampleGGX(vec2 xi, vec3 n, float roughness) {
        float a = roughness * roughness;
        float phi = 2.0 * PI * xi.x;
        float cos_t = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
        float sin_t = sqrt(max(1.0 - cos_t * cos_t, 0.0));
        vec3 h = vec3(cos(phi) * sin_t, sin(phi) * sin_t, cos_t);
        vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
        vec3 tangent = normalize(cross(up, n));
        vec3 bitangent = cross(n, tangent);
        return normalize(tangent * h.x + bitangent * h.y + n * h.z);
    }

    float dGGX(float nDotH, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float d = (nDotH * a2 - nDotH) * nDotH + 1.0;
        return a2 / max(PI * d * d, 1e-7);
    }

    void main() {
        vec3 n = normalize(vLocalPos);
        vec3 v = n;

        uint count = uint(max(sampleCount, 1));
        vec3 prefiltered = vec3(0.0);
        float weight = 0.0;

        for (uint i = 0u; i < count; ++i) {
            vec3 h = importanceSampleGGX(hammersley(i, count), n, roughness);
            vec3 l = normalize(2.0 * dot(v, h) * h - v);
            float nDotL = max(dot(n, l), 0.0);
            if (nDotL <= 0.0)
                continue;

            float nDotH = max(dot(n, h), 0.0);
            float vDotH = max(dot(v, h), 0.0);
            float mip = 0.0;
            if (roughness > 0.0) {
                float pdf = dGGX(nDotH, roughness) * nDotH / max(4.0 * vDotH, 1e-4);
                float saTexel = 4.0 * PI / (6.0 * envResolution * envResolution);
                float saSample = 1.0 / (float(count) * max(pdf, 1e-4));
                mip = 0.5 * log2(saSample / saTexel);
            }
            prefiltered += textureLod(envMap, l, mip).rgb * nDotL;
            weight += nDotL;
        }

        fragColor = vec4(prefiltered / max(weight, 1e-4), 1.0);
    }
)";
