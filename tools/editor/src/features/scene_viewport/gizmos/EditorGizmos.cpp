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

void EditorGizmos::toggleBoxCollider(EntityHandle objectId) {
    if (boxColliderEnabled_.contains(objectId))
        boxColliderEnabled_.erase(objectId);
    else
        boxColliderEnabled_.insert(objectId);
}

void EditorGizmos::toggleCapsuleCollider(EntityHandle objectId) {
    if (capsuleColliderEnabled_.contains(objectId))
        capsuleColliderEnabled_.erase(objectId);
    else
        capsuleColliderEnabled_.insert(objectId);
}

void EditorGizmos::toggleBVH() { bvhEnabled_ = !bvhEnabled_; }
