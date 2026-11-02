#pragma once

#include <glm/glm.hpp>

struct Material1 {
    glm::vec3 albedo{ 1.f, 0.f, 1.f };
    float metallic = 0.f;
    float roughness = 0.3f;
    float clearcoat = 1.f;
    float clearcoatRoughness = 0.1f;
};
