#pragma once
#include <imgui.h>
#include "../../controllers/SceneMutationsController.hpp"
#include "data/components/Camera.hpp"

class CameraWidget {
public:
    explicit CameraWidget(SceneMutationsController& mutations) : mutations_(mutations) {}

    void draw(Camera& c) const {
        if (!ImGui::BeginChild("Camera", {0, childHeight(5)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Camera");
        ImGui::Spacing();
        ImGui::DragFloat("FOV", &c.fov, 0.5f, 1.0f, 179.0f);
        trackDrag(c.fov);
        ImGui::DragFloat("Near Plane", &c.nearPlane, 0.01f, 0.001f, 1000.0f);
        trackDrag(c.nearPlane);
        ImGui::DragFloat("Far Plane", &c.farPlane, 0.5f, 0.1f, 10000.0f);
        trackDrag(c.farPlane);
        ImGui::Text("Aspect: %.3f", c.aspect);
        ImGui::EndChild();
    }

private:
    SceneMutationsController& mutations_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }

    template<typename T>
    void trackDrag(T& target) const {
        if (ImGui::IsItemActivated()) mutations_.beginEdit(target);
        if (ImGui::IsItemDeactivatedAfterEdit()) mutations_.commitEdit(target);
    }
};
