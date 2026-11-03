#pragma once

#include "core/render/shadow_map.h"

#include <glm/glm.hpp>

struct Light {
	glm::vec3 dir;
	glm::vec3 color;
	float intensity = 1.f;
	bool enabled = true;
	bool shadow = false;
	ShadowMap map;
	glm::mat4 viewProj{ 1.f };
};