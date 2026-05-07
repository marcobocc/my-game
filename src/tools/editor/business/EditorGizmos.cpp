#include "EditorGizmos.hpp"

void EditorGizmos::toggleAABB(EntityHandle objectId) {
    if (aabbEnabled_.contains(objectId))
        aabbEnabled_.erase(objectId);
    else
        aabbEnabled_.insert(objectId);
}

void EditorGizmos::toggleBoundingSphere(EntityHandle objectId) {
    if (boundingSpheresEnabled_.contains(objectId))
        boundingSpheresEnabled_.erase(objectId);
    else
        boundingSpheresEnabled_.insert(objectId);
}

void EditorGizmos::toggleBVH() { bvhEnabled_ = !bvhEnabled_; }
