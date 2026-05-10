#pragma once
#include <imgui.h>
#include "data/components/Camera.hpp"
#include "modules/scene/EntityMetadata.hpp"

class SceneMutations;

class CameraWidget {
public:
    explicit CameraWidget(SceneMutations& sceneMutations) : sceneMutations_(sceneMutations), lastObjectId_{} {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void draw(Camera& c) {
        ImGui::DragFloat("FOV", &c.fov, 0.5f, 1.0f, 179.0f);
        trackDrag();
        ImGui::DragFloat("Near Plane", &c.nearPlane, 0.01f, 0.001f, 1000.0f);
        trackDrag();
        ImGui::DragFloat("Far Plane", &c.farPlane, 0.5f, 0.1f, 10000.0f);
        trackDrag();
        ImGui::Text("Aspect: %.3f", c.aspect);
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
