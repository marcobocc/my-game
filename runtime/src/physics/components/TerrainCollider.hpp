#pragma once
#include "../../core/components/IComponent.hpp"

// Marks an entity's grid mesh (see terrain.md) as walkable terrain. The
// collision shape is the sculpted mesh itself, resolved as a heightfield —
// the geometry always comes from the entity's Renderer mesh, so there is
// nothing to configure.
struct TerrainCollider final : IComponent {
    nlohmann::json serialize() const override { return nlohmann::json::object(); }

    std::string typeName() const override { return "TerrainCollider"; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<TerrainCollider>(*this); }

    static TerrainCollider deserialize(const nlohmann::json&) { return TerrainCollider{}; }
};
