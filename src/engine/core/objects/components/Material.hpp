#pragma once
#include <glm/vec4.hpp>
#include <string>

struct Material {
    std::string shaderName{"unlit_textured"};
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    std::string textureName{"checkers"};
};
