#pragma once
#include <glm/vec4.hpp>
#include <optional>
#include <string>

struct Renderer {
    std::string meshName;
    std::string materialName;
    std::optional<glm::vec4> baseColorOverride;
    bool enabled = true;
};
