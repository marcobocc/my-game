#pragma once
#include <functional>
#include <unordered_set>
#include "modules/physics/collision_utils.hpp"
#include "modules/scene/World.hpp"

class PhysicsSystem {
public:
    using CollisionPair = std::pair<EntityHandle, EntityHandle>;
    enum class CollisionEventType { Enter, Stay, Exit };
    using CollisionCallback = std::function<void(const CollisionPair&)>;

    explicit PhysicsSystem(World& entityManager) : entityManager_(entityManager) {}

    void pause() { paused_ = true; }
    void resume() { paused_ = false; }

    static bool checkCollision(const BoxCollider& a, const Transform& ta, const BoxCollider& b, const Transform& tb) {
        return Physics::checkCollision(a, ta, b, tb);
    }

    std::optional<Physics::RaycastHit> raycast(const glm::vec3& origin, const glm::vec3& dir) const {
        auto objects = entityManager_.query<BoxCollider, Transform>();
        std::vector<std::tuple<EntityHandle, const BoxCollider*, const Transform*>> wrapped;
        for (auto& [entity, collPtr, transformPtr]: objects) {
            if (collPtr && transformPtr) {
                wrapped.emplace_back(entity, collPtr, transformPtr);
            }
        }
        return Physics::raycast(origin, dir, wrapped);
    }

    void onCollisionEnter(CollisionCallback cb) { enterCallbacks_.push_back(std::move(cb)); }
    void onCollisionStay(CollisionCallback cb) { stayCallbacks_.push_back(std::move(cb)); }
    void onCollisionExit(CollisionCallback cb) { exitCallbacks_.push_back(std::move(cb)); }

    void update() {
        if (paused_) return;
        auto objects = entityManager_.query<BoxCollider, Transform>();
        std::unordered_set<CollisionPair, PairHash> currentCollisions;
        for (size_t i = 0; i < objects.size(); ++i) {
            const auto& [idA, colliderAPtr, transformAPtr] = objects[i];
            if (!colliderAPtr || !transformAPtr) continue;
            for (size_t j = i + 1; j < objects.size(); ++j) {
                const auto& [idB, colliderBPtr, transformBPtr] = objects[j];
                if (!colliderBPtr || !transformBPtr) continue;
                if (checkCollision(*colliderAPtr, *transformAPtr, *colliderBPtr, *transformBPtr)) {
                    currentCollisions.insert(makeOrderedPair(idA, idB));
                }
            }
        }

        for (const auto& pair: currentCollisions) {
            if (prevCollisions_.contains(pair)) {
                for (auto& cb: stayCallbacks_)
                    cb(pair);
            } else {
                for (auto& cb: enterCallbacks_)
                    cb(pair);
            }
        }
        for (const auto& pair: prevCollisions_) {
            if (!currentCollisions.contains(pair)) {
                for (auto& cb: exitCallbacks_)
                    cb(pair);
            }
        }
        prevCollisions_ = std::move(currentCollisions);
    }

private:
    struct PairHash {
        std::size_t operator()(const CollisionPair& p) const {
            return std::hash<EntityHandle>{}(p.first) ^ (std::hash<EntityHandle>{}(p.second) << 1);
        }
    };

    static CollisionPair makeOrderedPair(const EntityHandle& a, const EntityHandle& b) {
        return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
    }

    bool paused_ = true;
    World& entityManager_;
    std::unordered_set<CollisionPair, PairHash> prevCollisions_;
    std::vector<CollisionCallback> enterCallbacks_;
    std::vector<CollisionCallback> stayCallbacks_;
    std::vector<CollisionCallback> exitCallbacks_;
};
