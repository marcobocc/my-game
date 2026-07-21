#pragma once
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "../graphics/components/Renderer.hpp"
#include "CollisionDetection.hpp"
#include "MeshColliderUtils.hpp"
#include "MeshCollisionDetection.hpp"
#include "TerrainHeightfield.hpp"
#include "components/BoxCollider.hpp"
#include "components/CapsuleCollider.hpp"
#include "components/MeshCollider.hpp"
#include "components/TerrainCollider.hpp"
#include "core/scene/TransformUtils.hpp"
#include "core/scene/World.hpp"

// Discrete penetration-resolution collision solver for a single moving entity
// (the character-controller pattern): computes the accumulated MTV to push an
// entity with a BoxCollider or CapsuleCollider out of all overlapping box
// colliders, mesh colliders (convex hull), and terrain heightfields.
//
// Owns a small cache of lazily-built TerrainHeightfields (keyed by mesh name),
// so it's meant to be a long-lived member (one per caller, e.g. LuaWorld)
// rather than constructed per call.
namespace Physics {
    using MeshLookup = std::function<const Mesh*(const std::string&)>;

    class CollisionResolver {
    public:
        // Returns the accumulated MTV to push `entity` out of all overlapping
        // colliders/terrain, or zero if there's no overlap or the entity has no
        // BoxCollider/CapsuleCollider. `entity`'s children are excluded from the check.
        glm::vec3 resolve(World& world, EntityHandle entity, const MeshLookup& meshLookup) const {
            const Actor* actor = world.getActor(entity);
            if (!actor) return glm::vec3(0.0f);
            const Transform* selfTransform = actor->getComponent<Transform>();
            if (!selfTransform) return glm::vec3(0.0f);

            const BoxCollider* selfBox = actor->getComponent<BoxCollider>();
            const CapsuleCollider* selfCapsule = actor->getComponent<CapsuleCollider>();
            if (!selfBox && !selfCapsule) return glm::vec3(0.0f);

            glm::mat4 selfMat = TransformUtils::resolveWorldMatrix(*selfTransform, world);
            std::vector<glm::vec3> pushes;

            auto boxes = world.query<BoxCollider, Transform>();
            for (const auto& [other, otherCollider, otherTransform]: boxes) {
                if (other == entity) continue;
                if (otherTransform->parent == entity) continue;
                glm::mat4 otherMat = TransformUtils::resolveWorldMatrix(*otherTransform, world);

                std::optional<glm::vec3> mtv;
                if (selfBox)
                    mtv = Physics::computeMTV(*selfBox, selfMat, *otherCollider, otherMat);
                else
                    mtv = Physics::computeCapsuleBoxMTV(*selfCapsule, selfMat, *otherCollider, otherMat);

                if (mtv) pushes.push_back(*mtv);
            }

            auto meshColliders = world.query<MeshCollider, Transform>();
            for (const auto& [other, otherCollider, otherTransform]: meshColliders) {
                if (other == entity) continue;
                if (otherTransform->parent == entity) continue;
                const Actor* otherActor = world.getActor(other);
                if (!otherActor) continue;
                std::string meshName = Physics::resolveMeshColliderMeshName(*otherActor, *otherCollider);
                const Mesh* mesh = (meshLookup && !meshName.empty()) ? meshLookup(meshName) : nullptr;
                Physics::ensureMeshColliderHull(*otherCollider, mesh);
                if (otherCollider->hull.vertices.empty()) continue;

                glm::mat4 otherMat = TransformUtils::resolveWorldMatrix(*otherTransform, world);

                std::optional<glm::vec3> mtv;
                if (selfBox)
                    mtv = Physics::computeBoxHullMTV(*selfBox, selfMat, otherCollider->hull, otherMat);
                else
                    mtv = Physics::computeCapsuleHullMTV(*selfCapsule, selfMat, otherCollider->hull, otherMat);

                if (mtv) pushes.push_back(*mtv);
            }

            auto terrains = world.query<TerrainCollider, Transform>();
            if (!terrains.empty()) {
                std::vector<glm::vec3> samplePoints = selfBox ? Physics::terrainSamplePoints(*selfBox, selfMat)
                                                              : Physics::terrainSamplePoints(*selfCapsule, selfMat);
                for (const auto& [other, terrainCollider, terrainTransform]: terrains) {
                    if (other == entity) continue;
                    const TerrainHeightfield* heightfield = heightfieldFor(world, other, meshLookup);
                    if (!heightfield) continue;
                    glm::mat4 terrainMat = TransformUtils::resolveWorldMatrix(*terrainTransform, world);
                    auto mtv = Physics::computeTerrainMTV(
                            samplePoints, *heightfield, terrainMat, glm::inverse(terrainMat));
                    if (mtv) pushes.push_back(*mtv);
                }
            }

            if (pushes.empty()) return glm::vec3(0.0f);

            // Find the push with the largest upward component — treat it as the primary ground push.
            // For remaining pushes, only accumulate horizontal (XZ) components to avoid
            // vertical fighting between adjacent colliders at seams.
            int primaryIdx = 0;
            for (int i = 1; i < static_cast<int>(pushes.size()); ++i)
                if (pushes[i].y > pushes[primaryIdx].y) primaryIdx = i;

            glm::vec3 result = pushes[primaryIdx];
            for (int i = 0; i < static_cast<int>(pushes.size()); ++i) {
                if (i == primaryIdx) continue;
                result.x += pushes[i].x;
                result.z += pushes[i].z;
            }
            return result;
        }

    private:
        // Heightfields are built lazily from the terrain's Renderer mesh and cached
        // per mesh name; mesh assets don't change while a simulation is running.
        const TerrainHeightfield*
        heightfieldFor(World& world, EntityHandle terrainEntity, const MeshLookup& meshLookup) const {
            const Actor* actor = world.getActor(terrainEntity);
            if (!actor || !meshLookup) return nullptr;
            const Renderer* renderer = actor->getComponent<Renderer>();
            if (!renderer) return nullptr;

            auto it = heightfields_.find(renderer->meshName);
            if (it == heightfields_.end()) {
                const Mesh* mesh = meshLookup(renderer->meshName);
                std::optional<TerrainHeightfield> built = mesh ? TerrainHeightfield::build(*mesh) : std::nullopt;
                it = heightfields_.emplace(renderer->meshName, std::move(built)).first;
            }
            return it->second ? &*it->second : nullptr;
        }

        mutable std::unordered_map<std::string, std::optional<TerrainHeightfield>> heightfields_;
    };
} // namespace Physics
