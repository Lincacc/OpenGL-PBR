#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <cstddef>

// 2 阶实球谐，世界空间 Y 朝上。系数顺序与 shader_code_0.h 的 evalSHIrradiance 一致：
// 0:L00  1:L1-1(y)  2:L10(z)  3:L11(x)
// 4:L2-2(xy)  5:L2-1(yz)  6:L20(3z²-1)  7:L21(xz)  8:L22(x²-y²)
// c[] 存的是辐照度系数 E_lm = A_l * L_lm，A_l 为 Lambert 余弦核的波段因子。
struct SH9 {
    glm::vec3 c[9]{};
};

inline void sh_basis(const glm::vec3& n, float y[9]) {
    const float x = n.x;
    const float yy = n.y;
    const float z = n.z;
    y[0] = 0.2820947918f;
    y[1] = 0.4886025119f * yy;
    y[2] = 0.4886025119f * z;
    y[3] = 0.4886025119f * x;
    y[4] = 1.0925484306f * x * yy;
    y[5] = 1.0925484306f * yy * z;
    y[6] = 0.3153915653f * (3.f * z * z - 1.f);
    y[7] = 1.0925484306f * x * z;
    y[8] = 0.5462742153f * (x * x - yy * yy);
}

inline SH9 sh_radiance_to_irradiance(const SH9& radiance) {
    SH9 e = radiance;
    const float a0 = glm::pi<float>();
    const float a1 = 2.f * glm::pi<float>() / 3.f;
    const float a2 = glm::pi<float>() / 4.f;
    e.c[0] *= a0;
    e.c[1] *= a1;
    e.c[2] *= a1;
    e.c[3] *= a1;
    e.c[4] *= a2;
    e.c[5] *= a2;
    e.c[6] *= a2;
    e.c[7] *= a2;
    e.c[8] *= a2;
    return e;
}

inline glm::vec3 sh_eval_irradiance(const SH9& sh, const glm::vec3& n) {
    float y[9];
    sh_basis(glm::normalize(n), y);
    glm::vec3 e(0.f);
    for (int i = 0; i < 9; ++i)
        e += sh.c[i] * y[i];
    return glm::max(e, glm::vec3(0.f));
}

// 无 HDR 时：+Y 半球为天空辐射亮度，-Y 半球为地面辐射亮度。
inline SH9 sh_irradiance_from_sky_ground(const glm::vec3& sky, const glm::vec3& ground) {
    SH9 radiance{};
    constexpr int kTheta = 32;
    constexpr int kPhi = 64;
    const float d_theta = glm::pi<float>() / float(kTheta);
    const float d_phi = 2.f * glm::pi<float>() / float(kPhi);

    for (int it = 0; it < kTheta; ++it) {
        const float theta = (float(it) + 0.5f) * d_theta;
        const float sin_t = std::sin(theta);
        const float cos_t = std::cos(theta);
        const float d_omega = sin_t * d_theta * d_phi;
        for (int ip = 0; ip < kPhi; ++ip) {
            const float phi = (float(ip) + 0.5f) * d_phi;
            const glm::vec3 w{
                sin_t * std::cos(phi),
                cos_t,
                sin_t * std::sin(phi)
            };
            const glm::vec3 L = (w.y >= 0.f) ? sky : ground;
            float y[9];
            sh_basis(w, y);
            for (int i = 0; i < 9; ++i)
                radiance.c[i] += L * y[i] * d_omega;
        }
    }
    return sh_radiance_to_irradiance(radiance);
}

// 等距柱状图约定与 shader_code_skybox.h 的 sampleEquirect 一致：
// v=0 为 +Y，v=1 为 -Y；u=0.5 为 +X。rgb 为线性 RGB 交错。
inline SH9 sh_irradiance_from_equirect(const float* rgb, int w, int h) {
    SH9 radiance{};
    if (rgb == nullptr || w <= 0 || h <= 0)
        return radiance;

    const float du = 2.f * glm::pi<float>() / float(w);
    const float dv = glm::pi<float>() / float(h);

    for (int y = 0; y < h; ++y) {
        const float v = (float(y) + 0.5f) / float(h);
        const float theta = v * glm::pi<float>();
        const float sin_t = std::sin(theta);
        const float cos_t = std::cos(theta);
        const float d_omega = sin_t * du * dv;
        if (d_omega <= 0.f)
            continue;
        for (int x = 0; x < w; ++x) {
            const float u = (float(x) + 0.5f) / float(w);
            const float phi = (u - 0.5f) * 2.f * glm::pi<float>();
            const glm::vec3 dir{
                sin_t * std::cos(phi),
                cos_t,
                sin_t * std::sin(phi)
            };
            const float* p = rgb + (size_t(y) * size_t(w) + size_t(x)) * 3;
            const glm::vec3 L(p[0], p[1], p[2]);
            float basis[9];
            sh_basis(dir, basis);
            for (int i = 0; i < 9; ++i)
                radiance.c[i] += L * basis[i] * d_omega;
        }
    }
    return sh_radiance_to_irradiance(radiance);
}
