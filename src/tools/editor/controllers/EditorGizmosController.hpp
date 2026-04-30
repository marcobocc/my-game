#pragma once
#include <string>
#include <unordered_set>
#include "GameEngine.hpp"

class EditorGizmosController {
public:
    explicit EditorGizmosController(GameEngine& engine) : engine_(engine) {}

    void enableAABB(const std::string& objectId) { aabbEnabled_.insert(objectId); }
    void disableAABB(const std::string& objectId) { aabbEnabled_.erase(objectId); }
    bool isAABBEnabled(const std::string& objectId) const { return aabbEnabled_.contains(objectId); }

    void draw() const {
        for (const auto& objectId: aabbEnabled_)
            engine_.GIZMOS_DrawObjectAABB(objectId, {0.0f, 1.0f, 0.0f});
    }

private:
    GameEngine& engine_;
    std::unordered_set<std::string> aabbEnabled_;
};
