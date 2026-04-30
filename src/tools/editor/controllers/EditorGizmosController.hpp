#pragma once
#include <optional>
#include <string>
#include <unordered_set>
#include "GameEngine.hpp"

class EditorGizmosController {
public:
    explicit EditorGizmosController(GameEngine& engine) : engine_(engine) {}

    void enableAABB(const std::string& objectId) { aabbEnabled_.insert(objectId); }
    void disableAABB(const std::string& objectId) { aabbEnabled_.erase(objectId); }
    bool isAABBEnabled(const std::string& objectId) const { return aabbEnabled_.contains(objectId); }

    void setSelectedObject(const std::optional<std::string>& objectId) { selectedObjectId_ = objectId; }
    void toggleBVH() { bvhEnabled_ = !bvhEnabled_; }

    void draw() const {
        if (selectedObjectId_) engine_.GIZMOS_DrawObjectTransform(*selectedObjectId_, 1.0f);

        for (const auto& objectId: aabbEnabled_)
            engine_.GIZMOS_DrawObjectAABB(objectId, {0.0f, 1.0f, 0.0f});

        if (bvhEnabled_) engine_.GIZMOS_DrawBVH({1.0f, 1.0f, 0.0f});
    }

private:
    GameEngine& engine_;
    std::optional<std::string> selectedObjectId_;
    std::unordered_set<std::string> aabbEnabled_;
    bool bvhEnabled_ = false;
};
