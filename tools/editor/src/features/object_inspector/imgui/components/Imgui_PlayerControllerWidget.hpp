#pragma once
#include <imgui.h>
#include <string>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/gameplay/PlayerController.hpp"

class RuntimeScene;

class Imgui_PlayerControllerWidget : public Imgui_InspectorWidget {
public:
    Imgui_PlayerControllerWidget(RuntimeScene& scene, std::string title) :
        Imgui_InspectorWidget(std::move(title), 3),
        scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void setComponent(const PlayerController& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    PlayerController component_{};

    void drawBody() override {
        if (!lastObjectId_) return;

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 150.0f);

        ImGui::TextUnformatted("Move Speed");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##moveSpeed", &component_.moveSpeed_, 0.1f, 0.1f, 50.0f)) {
            commitEdit();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("Mouse Sensitivity");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##mouseSensitivity", &component_.mouseSensitivity_, 0.01f, 0.01f, 1.0f)) {
            commitEdit();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("Jump Force");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##jumpForce", &component_.jumpForce_, 10.0f, 100.0f, 2000.0f)) {
            commitEdit();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("Gravity");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##gravity", &component_.gravity_, 0.1f, 0.1f, 50.0f)) {
            commitEdit();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("Mass");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##mass", &component_.mass_, 1.0f, 1.0f, 200.0f)) {
            commitEdit();
        }
        ImGui::NextColumn();

        ImGui::TextUnformatted("Height");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##height", &component_.height_, 0.1f, 0.1f, 5.0f)) {
            commitEdit();
        }

        ImGui::Columns(1);
    }

    void commitEdit() {
        if (!lastObjectId_) return;
        PlayerController snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<PlayerController>([snapshot](PlayerController& c) { c = snapshot; },
                                                   UndoHistory::randomGroupId("Edit PlayerController"));
    }
};
