#pragma once
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include "../../controllers/SceneMutationsController.hpp"
#include "data/components/Transform.hpp"

class TransformWidget {
public:
    explicit TransformWidget(SceneMutationsController& mutations) : mutations_(mutations) {}

    void draw(Transform& t) const {
        if (!ImGui::BeginChild("Transform", {0, childHeight(4)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Transform");
        ImGui::Spacing();
        ImGui::DragFloat3("Position", &t.position.x, 0.01f);
        trackDrag(t.position);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
        if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f)) t.rotation = glm::quat(glm::radians(euler));
        trackDrag(t.rotation);
        ImGui::DragFloat3("Scale", &t.scale.x, 0.01f);
        trackDrag(t.scale);
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
