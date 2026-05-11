#pragma once
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include "../ComponentContainer.hpp"
#include "modules/scene/components/Transform.hpp"

class SceneMutations;

class TransformWidget : public ComponentContainer {
public:
    explicit TransformWidget(SceneMutations& sceneMutations) :
        ComponentContainer("Transform", 5),
        sceneMutations_(sceneMutations) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(Transform& t) { component_ = &t; }

private:
    SceneMutations& sceneMutations_;
    std::optional<EntityHandle> lastObjectId_;
    Transform* component_ = nullptr;

    void drawBody() override {
        if (!component_) return;
        drawVec3Control("Position", component_->position, 0.01f);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(component_->rotation));

        if (drawVec3Control("Rotation", euler, 0.5f)) {
            component_->rotation = glm::quat(glm::radians(euler));
        }

        drawVec3Control("Scale", component_->scale, 0.01f);
    }

    bool drawVec3Control(const char* label, glm::vec3& value, float speed) {
        bool changed = false;
        ImGui::PushID(label);
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 70.0f);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        float width = (ImGui::GetContentRegionAvail().x - 8.0f) / 3.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
        ImGui::SetNextItemWidth(width);
        changed |= ImGui::InputFloat("##X", &value.x, 0, 0, "X: %.2f");
        trackEdit();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(width);
        changed |= ImGui::InputFloat("##Y", &value.y, 0, 0, "Y: %.2f");
        trackEdit();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(width);
        changed |= ImGui::InputFloat("##Z", &value.z, 0, 0, "Z: %.2f");
        trackEdit();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();

        return changed;
    }

    void trackEdit() {
        if (!lastObjectId_) return;
        if (ImGui::IsItemActivated()) sceneMutations_.beginEdit(*lastObjectId_);
        if (ImGui::IsItemDeactivatedAfterEdit()) sceneMutations_.commitEdit(*lastObjectId_);
    }
};
