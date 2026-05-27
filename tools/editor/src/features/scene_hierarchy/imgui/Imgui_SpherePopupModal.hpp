#pragma once
#include <imgui.h>
#include "../../../services/common_editing/SceneQuickActions.hpp"
#include "../../../styling/ImguiStyling.hpp"

class Imgui_SpherePopupModal {
public:
    Imgui_SpherePopupModal(SceneQuickActions& sceneQuickActions) :
        sphereResolution_(6),
        sceneQuickActions_(sceneQuickActions),
        showModal_(false) {}

    void requestCreation() { showModal_ = true; }

    void draw() {
        if (showModal_) {
            ImGui::OpenPopup("SphereResolutionModal");
            showModal_ = false;
        }
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImguiStyling::withPopupModal("SphereResolutionModal", [this] {
            ImGui::Text("Sphere Resolution");
            ImGui::SliderInt("##SphereResolution", reinterpret_cast<int*>(&sphereResolution_), 6, 24);

            if (ImGui::Button("Create", ImVec2(120, 0))) {
                sceneQuickActions_.createSphere(sphereResolution_);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
        });
    }

private:
    uint32_t sphereResolution_;
    SceneQuickActions& sceneQuickActions_;
    bool showModal_;
};
