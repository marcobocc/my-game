#pragma once
#include <unordered_set>
#include "modules/scene/EntityManager.hpp"

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

    void toggleBVH();
    bool bvhEnabled() const { return bvhEnabled_; }

private:
    std::unordered_set<EntityHandle> aabbEnabled_;
    std::unordered_set<EntityHandle> boundingSpheresEnabled_;
    bool bvhEnabled_ = false;
};
