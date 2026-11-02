#pragma once

#include <glm/glm.hpp>

struct Material {
    glm::vec3 albedo{ 1.f };
    float metallic = 0.f;
    float roughness = 0.1f;
};
