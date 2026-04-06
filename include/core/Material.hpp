#pragma once
#include <string>
#include <glm/glm.hpp>

struct Material {
    std::string name;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
};
