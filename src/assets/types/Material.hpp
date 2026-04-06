#pragma once
#include <glm/glm.hpp>
#include <string>

struct Material {
    std::string name{};
    std::string shaderPipelineName{};
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
};
