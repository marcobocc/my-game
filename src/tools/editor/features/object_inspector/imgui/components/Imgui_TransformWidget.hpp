#pragma once
#include <atomic>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/Transform.hpp"

class RuntimeScene;

class Imgui_TransformWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_TransformWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Transform", 5), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const Transform& t) { component_ = t; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    Transform component_{};
    std::string groupId_;

    void drawBody() override {
        if (!lastObjectId_) return;
        drawVec3Control("Position", component_.position, 0.01f);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(component_.rotation));
        if (drawVec3Control("Rotation", euler, 0.5f)) component_.rotation = glm::quat(glm::radians(euler));
        drawVec3Control("Scale", component_.scale, 0.01f);
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
        if (ImGui::IsItemActivated()) groupId_ = UndoHistory::randomGroupId("Set Transform");
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            Transform snapshot = component_;
            scene_.getObject(*lastObjectId_)
                    .mutateComponent<Transform>([snapshot](Transform& t) { t = snapshot; }, groupId_);
            groupId_.clear();
        }
    }
};
