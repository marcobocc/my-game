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
        if (!ImGui::BeginChild("Camera", {0, childHeight(5)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Camera");
        ImGui::Spacing();
        ImGui::DragFloat("FOV", &c.fov, 0.5f, 1.0f, 179.0f);
        trackDrag();
        ImGui::DragFloat("Near Plane", &c.nearPlane, 0.01f, 0.001f, 1000.0f);
        trackDrag();
        ImGui::DragFloat("Far Plane", &c.farPlane, 0.5f, 0.1f, 10000.0f);
        trackDrag();
        ImGui::Text("Aspect: %.3f", c.aspect);
        ImGui::EndChild();
    }

private:
    SceneMutations& sceneMutations_;
    std::optional<EntityHandle> lastObjectId_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }

    void trackDrag() {
        if (!lastObjectId_) return;
        if (ImGui::IsItemActivated()) sceneMutations_.beginEdit(*lastObjectId_);
        if (ImGui::IsItemDeactivatedAfterEdit()) sceneMutations_.commitEdit(*lastObjectId_);
    }
};
