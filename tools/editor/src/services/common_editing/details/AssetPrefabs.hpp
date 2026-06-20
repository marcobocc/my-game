#pragma once
#include <string>
#include "modules/asset_management/asset_types/Material.hpp"
#include "modules/asset_management/asset_types/Mesh.hpp"

namespace AssetPrefabs {
    Mesh sphere(uint32_t resolution, const std::string& name);
    Mesh cube(const std::string& name);
    Mesh plane(const std::string& name);
    Mesh capsule(uint32_t resolution, const std::string& name);
    Material solidColor(const std::string& name);
} // namespace AssetPrefabs
