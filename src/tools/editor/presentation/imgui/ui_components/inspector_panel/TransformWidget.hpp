#pragma once
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include "data/components/Transform.hpp"

class SceneMutations;

class TransformWidget {
public:
    explicit TransformWidget(SceneMutations& sceneMutations) : sceneMutations_(sceneMutations), lastObjectId_("") {}

    void setCurrentObjectId(const std::string& objectId) { lastObjectId_ = objectId; }

    void draw(Transform& t) {
        if (!ImGui::BeginChild("Transform", {0, childHeight(4)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Transform");
        ImGui::Spacing();
        ImGui::DragFloat3("Position", &t.position.x, 0.01f);
        trackDrag<glm::vec3>("Transform", t.position);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
        if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f)) t.rotation = glm::quat(glm::radians(euler));
        trackDrag<glm::quat>("Transform", t.rotation);
        ImGui::DragFloat3("Scale", &t.scale.x, 0.01f);
        trackDrag<glm::vec3>("Transform", t.scale);
        ImGui::EndChild();
    }

private:
    SceneMutations& sceneMutations_;
    std::string lastObjectId_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }

    template<typename T>
    void trackDrag(const std::string& componentType, T& target) {
        if (ImGui::IsItemActivated()) {
            sceneMutations_.beginEditByType(lastObjectId_, componentType);
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            sceneMutations_.endEditByType(lastObjectId_, componentType);
        }
    }
};
