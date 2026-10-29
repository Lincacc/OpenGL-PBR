#pragma once

#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
    glm::vec3 position{ 0.f, 2.f, 4.f };
    float fovDegree = 45.f;
    float aspect = 16.f / 9.f;
    float nearPlane = 0.1f;
    float farPlane = 100.f;

    glm::mat4 view() const {
        constexpr glm::vec3 target{ 0.f, 0.f, 0.f };
        constexpr glm::vec3 worldUp{ 0.f, 1.f, 0.f };
        if (glm::length(position - target) < 1e-4f) {
            throw std::runtime_error("camera position too close to origin");
        }

        return glm::lookAt(position, target, worldUp);
    }

    glm::mat4 project() const {
        if (aspect <= 0.f) {
            throw std::runtime_error("camera aspect must be > 0");
        }

        return glm::perspective(glm::radians(fovDegree), aspect, nearPlane, farPlane);
    }
};