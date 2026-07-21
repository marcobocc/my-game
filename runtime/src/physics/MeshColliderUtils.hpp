#pragma once
#include <string>
#include "../core/scene/Actor.hpp"
#include "../graphics/assets/Mesh.hpp"
#include "../graphics/components/Renderer.hpp"
#include "components/MeshCollider.hpp"

namespace Physics {

    // A MeshCollider with no explicit meshName follows the entity's rendered mesh, so
    // it automatically tracks Renderer changes (including Mesh Builder's fork-on-write,
    // which repoints Renderer::meshName without touching MeshCollider itself).
    inline std::string resolveMeshColliderMeshName(const Actor& actor, const MeshCollider& collider) {
        if (!collider.meshName.empty()) return collider.meshName;
        if (const auto* renderer = actor.getComponent<Renderer>()) return renderer->meshName;
        return {};
    }

    // Rebuilds the cached convex hull from `mesh` if the collider is marked dirty.
    // No-op if already up to date or if the mesh couldn't be resolved.
    inline void ensureMeshColliderHull(MeshCollider& collider, const Mesh* mesh) {
        if (!collider.hullDirty) return;
        if (!mesh) return;
        collider.hull = computeConvexHull(mesh->getPositions());
        collider.hullDirty = false;
    }

} // namespace Physics
