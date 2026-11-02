#pragma once

#include <glm/glm.hpp>

struct Material4 {
    glm::vec3 albedo{ 0.55f, 0.22f, 0.62f };
    float metallic = 0.f;
    float roughness = 0.7f;
    float specular = 0.5f;
    glm::vec3 fuzzColor{ 0.85f, 0.65f, 0.90f };
    float cloth = 1.f;
};
