#pragma once

#include <glm/glm.hpp>

struct Light {
	glm::vec3 dir;
	glm::vec3 color;
	float intensity = 1.f;
	bool enabled = true;
};