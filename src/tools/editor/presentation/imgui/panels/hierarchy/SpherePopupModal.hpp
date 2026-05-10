#pragma once
#include <functional>
#include <imgui.h>
#include "../../ImguiStyling.hpp"

class SpherePopupModal {
public:
    using OnSphereCreatedCallback = std::function<void(uint32_t)>;

    explicit SpherePopupModal(const OnSphereCreatedCallback& onSphereCreated = nullptr) :
        sphereResolution_(6),
        onSphereCreated_(onSphereCreated),
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
                if (onSphereCreated_) {
                    onSphereCreated_(sphereResolution_);
                }
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
    OnSphereCreatedCallback onSphereCreated_;
    bool showModal_;
};
