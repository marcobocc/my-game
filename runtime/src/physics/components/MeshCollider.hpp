#pragma once
#include <string>
#include "../../core/components/IComponent.hpp"
#include "../ConvexHull.hpp"

// Collision shape derived from a mesh asset's convex hull. If `meshName` is empty,
// the entity's Renderer::meshName is used instead (see MeshColliderUtils.hpp), so
// most mesh colliders need no configuration beyond adding the component.
//
// The hull is a runtime-only cache (never serialized). It starts dirty and is
// (re)built lazily on first use; edits to the source mesh asset mark it dirty again
// (see AssetStore::setOnAssetMutated wiring in EditorContainer), so the collider
// keeps following the mesh as it's reshaped in the Mesh Builder.
struct MeshCollider final : IComponent {
    std::string meshName;

    ConvexHull hull;
    bool hullDirty = true;

    nlohmann::json serialize() const override { return {{"meshName", meshName}}; }
    std::string typeName() const override { return "MeshCollider"; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<MeshCollider>(*this); }

    static MeshCollider deserialize(const nlohmann::json& j) {
        MeshCollider m{};
        m.meshName = j.value("meshName", std::string{});
        return m;
    }
};
