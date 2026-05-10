#pragma once
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include "data/components/Transform.hpp"
#include "modules/scene/EntityMetadata.hpp"

class SceneMutations;

class TransformWidget {
public:
    explicit TransformWidget(SceneMutations& sceneMutations) : sceneMutations_(sceneMutations) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void draw(Transform& t) {
        ImGui::DragFloat3("Position", &t.position.x, 0.01f);
        trackDrag();
        glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
        if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f)) t.rotation = glm::quat(glm::radians(euler));
        trackDrag();
        ImGui::DragFloat3("Scale", &t.scale.x, 0.01f);
        trackDrag();
    }

private:
    SceneMutations& sceneMutations_;
    std::optional<EntityHandle> lastObjectId_;

    void trackDrag() {
        if (!lastObjectId_) return;
        if (ImGui::IsItemActivated()) sceneMutations_.beginEdit(*lastObjectId_);
        if (ImGui::IsItemDeactivatedAfterEdit()) sceneMutations_.commitEdit(*lastObjectId_);
    }
};
