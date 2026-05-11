#pragma once
#include <imgui.h>
#include "../ComponentContainer.hpp"
#include "structs/components/Light.hpp"

class SceneMutations;

class LightWidget : public ComponentContainer {
public:
    explicit LightWidget(SceneMutations& sceneMutations) :
        ComponentContainer("Light", 6),
        sceneMutations_(sceneMutations) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(Light& c) { component_ = &c; }

private:
    SceneMutations& sceneMutations_;
    std::optional<EntityHandle> lastObjectId_;
    Light* component_ = nullptr;

    void drawBody() override {
        if (!component_) return;

        drawRow("Type", [&] {
            const char* items[] = {"Directional"};
            int currentType = static_cast<int>(component_->type);
            if (ImGui::Combo("##type", &currentType, items, IM_ARRAYSIZE(items))) {
                component_->type = static_cast<LightType>(currentType);
                trackEdit();
            }
        });

        drawRow("Intensity", [&] {
            if (ImGui::DragFloat("##intensity", &component_->intensity, 0.1f, 0.0f, 10.0f)) trackEdit();
        });
    }

    template<typename Fn>
    void drawRow(const char* label, Fn&& fn) {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 90.0f);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(availWidth * 0.5f);
        fn();
        ImGui::PopStyleVar();
        ImGui::Columns(1);
    }

    void trackEdit() {
        if (!lastObjectId_) return;
        if (ImGui::IsItemActivated()) sceneMutations_.beginEdit(*lastObjectId_);
        if (ImGui::IsItemDeactivatedAfterEdit()) sceneMutations_.commitEdit(*lastObjectId_);
    }
};
