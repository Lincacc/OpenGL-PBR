#pragma once

#include <string>

const std::string vShaderCode = R"(
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

const std::string fShaderCode = R"(
    #version 460 core

    in vec3 vWorldPos;
    in vec3 vNormal;

    uniform vec3 cameraPos;
    uniform vec3 lightDir;     
    uniform vec3 lightColor;  
    uniform vec3 albedo;        
    uniform vec3 matParams;     
    uniform vec3 shIrradiance[9];

    layout(location = 0) out vec4 fragColor;

    const float PI = 3.14159265359;

    vec3 evalSHIrradiance(vec3 n) {
        float x = n.x, y = n.y, z = n.z;
        vec3 E = shIrradiance[0] * 0.2820947918;
        E += shIrradiance[1] * (0.4886025119 * y);
        E += shIrradiance[2] * (0.4886025119 * z);
        E += shIrradiance[3] * (0.4886025119 * x);
        E += shIrradiance[4] * (1.0925484306 * x * y);
        E += shIrradiance[5] * (1.0925484306 * y * z);
        E += shIrradiance[6] * (0.3153915653 * (3.0 * z * z - 1.0));
        E += shIrradiance[7] * (1.0925484306 * x * z);
        E += shIrradiance[8] * (0.5462742153 * (x * x - y * y));
        return max(E, vec3(0.0));
    }

    float D_GGX(float NdotH, float a) {
        float a2 = a * a;
        float d = (NdotH * a2 - NdotH) * NdotH + 1.0;
        return a2 / max(PI * d * d, 1e-7);
    }

    float G_Smith(float NdotV, float NdotL, float roughness) {
        float r = roughness + 1.0;
        float k = (r * r) / 8.0;
        float gV = NdotV / (NdotV * (1.0 - k) + k);
        float gL = NdotL / (NdotL * (1.0 - k) + k);
        return gV * gL;
    }

    vec3 F_Schlick(float cosTheta, vec3 F0) {
        return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    }

    void main() {
        vec3 N = normalize(vNormal);
        vec3 V = normalize(cameraPos - vWorldPos);
        vec3 L = normalize(lightDir);
        vec3 H = normalize(V + L);

        float metallic  = clamp(matParams.x, 0.0, 1.0);
        float roughness = clamp(matParams.y, 0.045, 1.0);
        float ao        = clamp(matParams.z, 0.0, 1.0);
        float a         = roughness * roughness;

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 1e-4);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 F  = F_Schlick(VdotH, F0);

        float D = D_GGX(NdotH, a);
        float G = G_Smith(NdotV, NdotL, roughness);
        vec3  spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-4);

        vec3 kD = (1.0 - F) * (1.0 - metallic);
        vec3 diffuse = kD * albedo / PI;

        vec3 direct = (diffuse + spec) * lightColor * NdotL;

        vec3 F_ibl = F_Schlick(NdotV, F0);
        vec3 kD_ibl = (1.0 - F_ibl) * (1.0 - metallic);
        vec3 ambient = kD_ibl * albedo / PI * evalSHIrradiance(N) * ao;

        vec3 color = ambient + direct;
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));
        fragColor = vec4(color, 1.0);
    }
)";