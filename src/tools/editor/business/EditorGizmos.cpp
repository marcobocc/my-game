#include "EditorGizmos.hpp"

void EditorGizmos::toggleAABB(const std::string& objectId) {
    if (aabbEnabled_.contains(objectId)) {
        aabbEnabled_.erase(objectId);
    } else {
        aabbEnabled_.insert(objectId);
    }
}

void EditorGizmos::toggleBoundingSphere(const std::string& objectId) {
    if (boundingSpheresEnabled_.contains(objectId)) {
        boundingSpheresEnabled_.erase(objectId);
    } else {
        boundingSpheresEnabled_.insert(objectId);
    }
}

void EditorGizmos::toggleBVH() { bvhEnabled_ = !bvhEnabled_; }
