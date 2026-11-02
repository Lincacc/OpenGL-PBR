#pragma once

#include <glm/glm.hpp>

struct Material2 {
    glm::vec3 albedo{ 1.f };
    float roughness = 0.045f;
    float ior = 1.5f;
    float transmission = 1.f;
    glm::vec3 attenuationColor{ 0.75f, 0.93f, 0.82f };
    float attenuationDistance = 0.45f;
};
