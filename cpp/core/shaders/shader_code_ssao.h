#pragma once

#include <string>

const std::string vSsaoShaderCode = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;

    out vec2 vUV;

    void main() {
        vUV = aPos.xy * 0.5 + 0.5;
        gl_Position = vec4(aPos, 1.0);
    }
)";

const std::string fSsaoShaderCode = R"(
    #version 460 core

    in vec2 vUV;

    uniform sampler2D depthMap;
    uniform sampler2D noiseMap;
    uniform vec3 samples[64];
    uniform mat4 projectionMatrix;
    uniform mat4 invProjection;
    uniform vec2 noiseScale;
    uniform float radius;
    uniform float bias;
    uniform int kernelSize;

    layout(location = 0) out float fragAo;

    vec3 viewPosFromDepth(vec2 uv) {
        float depth = texture(depthMap, uv).r;
        vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
        vec4 view = invProjection * clip;
        return view.xyz / max(view.w, 1e-6);
    }

    void main() {
        float depth = texture(depthMap, vUV).r;
        if (depth > 0.999) {
            fragAo = 1.0;
            return;
        }

        vec3 fragPos = viewPosFromDepth(vUV);
        vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));
        if (dot(n, -fragPos) < 0.0)
            n = -n;

        vec3 randomVec = texture(noiseMap, vUV * noiseScale).xyz;
        vec3 tangent = normalize(randomVec - n * dot(randomVec, n));
        vec3 bitangent = cross(n, tangent);
        mat3 TBN = mat3(tangent, bitangent, n);

        int count = clamp(kernelSize, 1, 64);
        float occlusion = 0.0;
        for (int i = 0; i < count; ++i) {
            vec3 samplePos = fragPos + TBN * samples[i] * radius;
            vec4 offset = projectionMatrix * vec4(samplePos, 1.0);
            offset.xyz /= max(offset.w, 1e-6);
            vec2 sampleUV = offset.xy * 0.5 + 0.5;
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
                continue;

            float sampleDepth = viewPosFromDepth(sampleUV).z;
            float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - sampleDepth), 1e-4));
            occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
        }
        fragAo = 1.0 - occlusion / float(count);
    }
)";

const std::string fSsaoBlurShaderCode = R"(
    #version 460 core

    in vec2 vUV;

    uniform sampler2D ssaoMap;
    uniform sampler2D depthMap;
    uniform mat4 invProjection;
    uniform vec2 texelSize;

    layout(location = 0) out float fragAo;

    vec3 viewPosFromDepth(vec2 uv) {
        float depth = texture(depthMap, uv).r;
        vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
        vec4 view = invProjection * clip;
        return view.xyz / max(view.w, 1e-6);
    }

    void main() {
        float centerZ = viewPosFromDepth(vUV).z;
        float result = 0.0;
        float wsum = 0.0;
        for (int x = -2; x < 2; ++x) {
            for (int y = -2; y < 2; ++y) {
                vec2 uv = vUV + vec2(float(x), float(y)) * texelSize;
                float ao = texture(ssaoMap, uv).r;
                float z = viewPosFromDepth(uv).z;
                float w = 1.0 / (1.0 + 20.0 * abs(centerZ - z));
                result += ao * w;
                wsum += w;
            }
        }
        fragAo = result / max(wsum, 1e-5);
    }
)";
