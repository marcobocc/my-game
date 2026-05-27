#pragma once
#include <atomic>
#include <imgui.h>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/BoxCollider.hpp"

class RuntimeScene;

class Imgui_BoxColliderWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_BoxColliderWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Box Collider", 4), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const BoxCollider& b) { component_ = b; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    BoxCollider component_{};
    std::string groupId_;

    void drawBody() override {
        if (!lastObjectId_) return;
        vec3Row("Center", component_.center, 0.01f);
        vec3Row("Half Extents", component_.halfExtents, 0.01f);
    }

    template<typename Vec>
    void vec3Row(const std::string& label, Vec& v, float speed) {
        ImGui::PushID(label.c_str());
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 110.0f);
        ImGui::TextUnformatted(label.c_str());
        ImGui::NextColumn();

        float w = (ImGui::GetContentRegionAvail().x - 8.0f) / 3.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
        ImGui::SetNextItemWidth(w);
        ImGui::DragFloat("##x", &v.x, speed);
        trackDrag(UndoHistory::randomGroupId("Set " + label));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(w);
        ImGui::DragFloat("##y", &v.y, speed);
        trackDrag(UndoHistory::randomGroupId("Set " + label));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(w);
        ImGui::DragFloat("##z", &v.z, speed);
        trackDrag(UndoHistory::randomGroupId("Set " + label));
        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
    }

    void trackDrag(const std::string& groupId) {
        if (!lastObjectId_) return;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            BoxCollider snapshot = component_;
            scene_.getObject(*lastObjectId_)
                    .mutateComponent<BoxCollider>([snapshot](BoxCollider& b) { b = snapshot; }, groupId);
        }
    }
};
