#pragma once

#include <glm/glm.hpp>

struct Material2 {
    glm::vec3 albedo{ 1.f };
    float roughness = 0.045f;
    float ior = 1.5f;
    float transmission = 1.f;
};
