#pragma once
#include <string>
#include "../../../../../../runtime/src/graphics/assets/Material.hpp"
#include "graphics/assets/Mesh.hpp"

namespace AssetPrefabs {
    Mesh sphere(uint32_t resolution, const std::string& name);
    Mesh cube(const std::string& name);
    Mesh plane(const std::string& name);
    Mesh capsule(uint32_t resolution, const std::string& name);
    Material solidColor(const std::string& name);
} // namespace AssetPrefabs
