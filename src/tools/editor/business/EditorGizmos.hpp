#pragma once
#include <string>
#include <unordered_set>

class EditorGizmos {
public:
    void toggleAABB(const std::string& objectId);
    bool aabbEnabled(const std::string& objectId) const { return aabbEnabled_.contains(objectId); }
    const std::unordered_set<std::string>& getObjectsWithAABBEnabled() const { return aabbEnabled_; }

    void toggleBoundingSphere(const std::string& objectId);
    bool boundingSphereEnabled(const std::string& objectId) const { return boundingSpheresEnabled_.contains(objectId); }
    const std::unordered_set<std::string>& getObjectsWithBoundingSpheresEnabled() const {
        return boundingSpheresEnabled_;
    }

    void toggleBVH();
    bool bvhEnabled() const { return bvhEnabled_; }

private:
    std::unordered_set<std::string> aabbEnabled_;
    std::unordered_set<std::string> boundingSpheresEnabled_;
    bool bvhEnabled_ = false;
};
