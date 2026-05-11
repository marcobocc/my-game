#pragma once
#include <imgui.h>
#include "../ComponentContainer.hpp"
#include "modules/scene/components/Camera.hpp"

class SceneMutations;

class CameraWidget : public ComponentContainer {
public:
    explicit CameraWidget(SceneMutations& sceneMutations) :
        ComponentContainer("Camera", 8),
        sceneMutations_(sceneMutations) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(Camera& c) { component_ = &c; }

private:
    SceneMutations& sceneMutations_;
    std::optional<EntityHandle> lastObjectId_;
    Camera* component_ = nullptr;

    void drawBody() override {
        if (!component_) return;
        drawRow("FOV", [&] {
            if (ImGui::DragFloat("##fov", &component_->fov, 0.5f, 1.0f, 179.0f)) trackDrag();
        });
        drawRow("Near Plane", [&] {
            if (ImGui::DragFloat("##near", &component_->nearPlane, 0.01f, 0.001f, 1000.0f)) trackDrag();
        });
        drawRow("Far Plane", [&] {
            if (ImGui::DragFloat("##far", &component_->farPlane, 0.5f, 0.1f, 10000.0f)) trackDrag();
        });
        ImGui::Separator();
        ImGui::Text("Aspect: %.3f", component_->aspect);
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

    void trackDrag() {
        if (!lastObjectId_) return;
        if (ImGui::IsItemActivated()) sceneMutations_.beginEdit(*lastObjectId_);
        if (ImGui::IsItemDeactivatedAfterEdit()) sceneMutations_.commitEdit(*lastObjectId_);
    }
};
