#pragma once

#include <glm/glm.hpp>

struct Material3 {
    glm::vec3 albedo{ 0.55f, 0.74f, 0.58f };
    float metallic = 0.f;
    float roughness = 0.18f;
    float specular = 0.78f;
    glm::vec3 subsurfaceColor{ 0.12f, 0.42f, 0.22f };
    float opacity = 0.55f;
};
