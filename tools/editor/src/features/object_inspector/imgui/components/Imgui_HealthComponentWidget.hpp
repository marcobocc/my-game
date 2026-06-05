#pragma once
#include <imgui.h>
#include <string>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/gameplay/HealthComponent.hpp"

class RuntimeScene;

class Imgui_HealthComponentWidget : public Imgui_InspectorWidget {
public:
    Imgui_HealthComponentWidget(RuntimeScene& scene, std::string title) :
        Imgui_InspectorWidget(std::move(title), 3),
        scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void setComponent(const HealthComponent& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    HealthComponent component_{};

    void drawBody() override {
        if (!lastObjectId_) return;

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 100.0f);

        ImGui::TextUnformatted("Health");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##hp", &component_.hp_, 1.0f, 0.0f, component_.maxHp_)) {
            commitEdit();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("Max Health");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##maxHp", &component_.maxHp_, 1.0f, 1.0f, 1000.0f)) {
            commitEdit();
        }

        ImGui::Columns(1);
    }

    void commitEdit() {
        if (!lastObjectId_) return;
        HealthComponent snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<HealthComponent>([snapshot](HealthComponent& c) { c = snapshot; },
                                                  UndoHistory::randomGroupId("Edit HealthComponent"));
    }
};
