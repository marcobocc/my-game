#pragma once
#include <imgui.h>
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/Camera.hpp"

class Imgui_CameraWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_CameraWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Camera", 10), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const Camera& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    Camera component_{};
    std::string groupId_;

    void drawBody() override {
        if (!lastObjectId_) return;
        bool isActive = scene_.isActiveCamera(*lastObjectId_);
        if (ImGui::Checkbox("Active Camera", &isActive)) {
            if (isActive)
                scene_.setActiveCamera(*lastObjectId_);
            else
                scene_.clearActiveCamera();
        }
        ImGui::Spacing();
        drawRow("FOV", [&] {
            ImGui::DragFloat("##fov", &component_.fov, 0.5f, 1.0f, 179.0f);
            trackDrag(UndoHistory::randomGroupId("Set FOV"));
        });
        drawRow("Near Plane", [&] {
            ImGui::DragFloat("##near", &component_.nearPlane, 0.01f, 0.001f, 1000.0f);
            trackDrag(UndoHistory::randomGroupId("Set Near Plane"));
        });
        drawRow("Far Plane", [&] {
            ImGui::DragFloat("##far", &component_.farPlane, 0.5f, 0.1f, 10000.0f);
            trackDrag(UndoHistory::randomGroupId("Set Far Plane"));
        });
        ImGui::Separator();
        ImGui::Text("Aspect: %.3f", component_.aspect);
    }

    template<typename Fn>
    void drawRow(const char* label, Fn&& fn) {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 90.0f);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
        fn();
        ImGui::PopStyleVar();
        ImGui::Columns(1);
    }

    void trackDrag(const std::string& groupId) {
        if (!lastObjectId_) return;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Camera snapshot = component_;
            scene_.getObject(*lastObjectId_).mutateComponent<Camera>([snapshot](Camera& c) { c = snapshot; }, groupId);
        }
    }
};
