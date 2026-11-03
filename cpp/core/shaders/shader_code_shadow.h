#pragma once

#include <string>

const std::string vShaderCodeShadow = R"(
    #version 460 core

    layout(location = 0) in vec3 aPos;

    uniform mat4 lightViewProj;
    uniform mat3 meshRotation;

    void main() {
        vec3 worldPos = meshRotation * aPos;
        gl_Position = lightViewProj * vec4(worldPos, 1.0);
    }
)";

const std::string fShaderCodeShadow = R"(
    #version 460 core

    void main() {}
)";
