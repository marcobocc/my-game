#pragma once
#include <imgui.h>
#include "../../ImguiStyling.hpp"

class SceneMutations;
class ObjectBuilder;

class SpherePopupModal {
public:
    SpherePopupModal(SceneMutations& sceneMutations, ObjectBuilder& objectBuilder) :
        sphereResolution_(6),
        sceneMutations_(sceneMutations),
        objectBuilder_(objectBuilder),
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
                sceneMutations_.createObject(objectBuilder_.sphere(sphereResolution_));
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
    SceneMutations& sceneMutations_;
    ObjectBuilder& objectBuilder_;
    bool showModal_;
};
