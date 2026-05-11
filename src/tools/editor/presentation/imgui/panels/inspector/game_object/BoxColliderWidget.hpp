#pragma once
#include <imgui.h>
#include "../ComponentContainer.hpp"
#include "structs/components/BoxCollider.hpp"

class SceneMutations;

class BoxColliderWidget : public ComponentContainer {
public:
    explicit BoxColliderWidget(SceneMutations& sceneMutations) :
        ComponentContainer("Box Collider", 4),
        sceneMutations_(sceneMutations) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(BoxCollider& b) { component_ = &b; }

private:
    SceneMutations& sceneMutations_;
    std::optional<EntityHandle> lastObjectId_;
    BoxCollider* component_ = nullptr;

    void drawBody() override {
        if (!component_) return;
        vec3Row("Center", component_->center, 0.01f);
        vec3Row("Half Extents", component_->halfExtents, 0.01f);
    }

    template<typename Vec>
    bool vec3Row(const char* label, Vec& v, float speed) {
        bool changed = false;

        ImGui::PushID(label);
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 110.0f);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();

        float w = (ImGui::GetContentRegionAvail().x - 8.0f) / 3.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
        ImGui::SetNextItemWidth(w);
        changed |= ImGui::DragFloat("##x", &v.x, speed);
        trackDrag();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(w);
        changed |= ImGui::DragFloat("##y", &v.y, speed);
        trackDrag();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(w);
        changed |= ImGui::DragFloat("##z", &v.z, speed);
        trackDrag();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
    }

    void trackDrag() {
        if (!lastObjectId_) return;
        if (ImGui::IsItemActivated()) sceneMutations_.beginEdit(*lastObjectId_);
        if (ImGui::IsItemDeactivatedAfterEdit()) sceneMutations_.commitEdit(*lastObjectId_);
    }
};
