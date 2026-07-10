#pragma once
#include <unordered_set>
#include "core/scene/EntityHandle.hpp"

class EditorGizmos {
public:
    void toggleAABB(EntityHandle objectId);
    bool aabbEnabled(EntityHandle objectId) const { return aabbEnabled_.contains(objectId); }
    const std::unordered_set<EntityHandle>& getObjectsWithAABBEnabled() const { return aabbEnabled_; }

    void toggleBoundingSphere(EntityHandle objectId);
    bool boundingSphereEnabled(EntityHandle objectId) const { return boundingSpheresEnabled_.contains(objectId); }
    const std::unordered_set<EntityHandle>& getObjectsWithBoundingSpheresEnabled() const {
        return boundingSpheresEnabled_;
    }

    void toggleBoxCollider(EntityHandle objectId);
    bool boxColliderEnabled(EntityHandle objectId) const { return boxColliderEnabled_.contains(objectId); }
    const std::unordered_set<EntityHandle>& getObjectsWithBoxColliderEnabled() const { return boxColliderEnabled_; }

    void toggleCapsuleCollider(EntityHandle objectId);
    bool capsuleColliderEnabled(EntityHandle objectId) const { return capsuleColliderEnabled_.contains(objectId); }
    const std::unordered_set<EntityHandle>& getObjectsWithCapsuleColliderEnabled() const {
        return capsuleColliderEnabled_;
    }

    void toggleBVH();
    bool bvhEnabled() const { return bvhEnabled_; }

private:
    std::unordered_set<EntityHandle> aabbEnabled_;
    std::unordered_set<EntityHandle> boundingSpheresEnabled_;
    std::unordered_set<EntityHandle> boxColliderEnabled_;
    std::unordered_set<EntityHandle> capsuleColliderEnabled_;
    bool bvhEnabled_ = false;
};
