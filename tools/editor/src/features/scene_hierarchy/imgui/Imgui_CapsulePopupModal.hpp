#pragma once
#include <imgui.h>
#include "../../../services/common_editing/SceneQuickActions.hpp"
#include "../../../styling/ImguiStyling.hpp"

class Imgui_CapsulePopupModal {
public:
    Imgui_CapsulePopupModal(SceneQuickActions& sceneQuickActions) :
        resolution_(6),
        sceneQuickActions_(sceneQuickActions),
        showModal_(false) {}

    void requestCreation() { showModal_ = true; }

    void draw() {
        if (showModal_) {
            ImGui::OpenPopup("CapsuleResolutionModal");
            showModal_ = false;
        }
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImguiStyling::withPopupModal("CapsuleResolutionModal", [this] {
            ImGui::Text("Capsule Resolution");
            ImGui::SliderInt("##CapsuleResolution", reinterpret_cast<int*>(&resolution_), 6, 24);

            if (ImGui::Button("Create", ImVec2(120, 0))) {
                sceneQuickActions_.createCapsule(resolution_);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
        });
    }

private:
    uint32_t resolution_;
    SceneQuickActions& sceneQuickActions_;
    bool showModal_;
};
