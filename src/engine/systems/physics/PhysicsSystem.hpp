#pragma once
#include <functional>
#include <unordered_set>
#include "systems/physics/collision_utils.hpp"
#include "systems/scene/Scene.hpp"

class PhysicsSystem {
public:
    using ObjectId = std::string;
    using CollisionPair = std::pair<ObjectId, ObjectId>;
    enum class CollisionEventType { Enter, Stay, Exit };
    using CollisionCallback = std::function<void(const CollisionPair&)>;

    explicit PhysicsSystem(Scene& scene) : scene_(scene) {}

    static bool checkCollision(const BoxCollider& a, const Transform& ta, const BoxCollider& b, const Transform& tb) {
        return Physics::checkCollision(a, ta, b, tb);
    }

    std::optional<Physics::RaycastHit> raycast(const glm::vec3& origin, const glm::vec3& dir) const {
        auto objects = scene_.getObjectsWith<BoxCollider, Transform>();
        return Physics::raycast(origin, dir, objects);
    }

    void onCollisionEnter(CollisionCallback cb) { enterCallbacks_.push_back(std::move(cb)); }
    void onCollisionStay(CollisionCallback cb) { stayCallbacks_.push_back(std::move(cb)); }
    void onCollisionExit(CollisionCallback cb) { exitCallbacks_.push_back(std::move(cb)); }

    void update() {
        auto objects = scene_.getObjectsWith<BoxCollider, Transform>();
        std::unordered_set<CollisionPair, PairHash> currentCollisions;
        for (size_t i = 0; i < objects.size(); ++i) {
            const auto& [idA, colliderA, transformA] = objects[i];
            for (size_t j = i + 1; j < objects.size(); ++j) {
                const auto& [idB, colliderB, transformB] = objects[j];
                if (checkCollision(colliderA, transformA, colliderB, transformB)) {
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
            return std::hash<std::string>{}(p.first) ^ std::hash<std::string>{}(p.second) << 1;
        }
    };

    static CollisionPair makeOrderedPair(const ObjectId& a, const ObjectId& b) {
        return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
    }

    Scene& scene_;
    std::unordered_set<CollisionPair, PairHash> prevCollisions_;
    std::vector<CollisionCallback> enterCallbacks_;
    std::vector<CollisionCallback> stayCallbacks_;
    std::vector<CollisionCallback> exitCallbacks_;
};
