#pragma once
#include <imgui.h>
#include "data/components/BoxCollider.hpp"
#include "modules/scene/EntityMetadata.hpp"

class SceneMutations;

class BoxColliderWidget {
public:
    explicit BoxColliderWidget(SceneMutations& sceneMutations) : sceneMutations_(sceneMutations) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void draw(BoxCollider& b) {
        ImGui::DragFloat3("Center", &b.center.x, 0.01f);
        trackDrag();
        ImGui::DragFloat3("Half Extents", &b.halfExtents.x, 0.01f);
        trackDrag();
    }

private:
    SceneMutations& sceneMutations_;
    std::optional<EntityHandle> lastObjectId_;

    void trackDrag() {
        if (!lastObjectId_) return;
        if (ImGui::IsItemActivated()) sceneMutations_.beginEdit(*lastObjectId_);
        if (ImGui::IsItemDeactivatedAfterEdit()) sceneMutations_.commitEdit(*lastObjectId_);
    }
};
