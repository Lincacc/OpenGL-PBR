#pragma once

#include <glm/glm.hpp>

struct Material5 {
    glm::vec3 albedo{ 0.75f };
    glm::vec3 rim{ 0.15f };
    float power = 1.f;
};
