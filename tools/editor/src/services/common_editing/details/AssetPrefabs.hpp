#pragma once
#include <string>
#include "../../../../../../runtime/src/graphics/assets/Material.hpp"
#include "graphics/assets/Mesh.hpp"
#include "graphics/assets/Texture.hpp"

namespace AssetPrefabs {
    Mesh sphere(uint32_t resolution, const std::string& name);
    Mesh cube(const std::string& name);
    Mesh plane(const std::string& name);
    Mesh capsule(uint32_t resolution, const std::string& name);
    // Flat grid of resolution x resolution quads, centered at the origin, spanning worldSize on X/Z.
    Mesh terrainMesh(uint32_t resolution, float worldSize, const std::string& name);
    Material solidColor(const std::string& name);
    // RGBA8 texture where layer 0 (R channel) is fully weighted everywhere, so a freshly painted
    // terrain renders as layerMaterial0 until the user paints other layers in.
    Texture splatMap(uint32_t resolution, const std::string& name);
    Material terrainBlend(const std::string& name, const std::string& splatMapTexture);
} // namespace AssetPrefabs
