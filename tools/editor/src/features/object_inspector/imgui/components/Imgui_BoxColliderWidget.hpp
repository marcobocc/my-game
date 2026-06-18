#pragma once
#include <optional>
#include <string>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/BoxCollider.hpp"

class RuntimeScene;

class Imgui_BoxColliderWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_BoxColliderWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Box Collider"), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const BoxCollider& b) { component_ = b; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    BoxCollider component_{};

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<Vec3Property>(
                "Center",
                [this] { return component_.center; },
                [this](glm::vec3 v) {
                    component_.center = v;
                    if (ImGui::IsItemDeactivatedAfterEdit()) commit("Set Center");
                },
                0.01f);

        add<Vec3Property>(
                "Half Extents",
                [this] { return component_.halfExtents; },
                [this](glm::vec3 v) {
                    component_.halfExtents = v;
                    if (ImGui::IsItemDeactivatedAfterEdit()) commit("Set Half Extents");
                },
                0.01f);
    }

    void commit(const char* label) {
        if (!lastObjectId_) return;
        BoxCollider snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<BoxCollider>([snapshot](BoxCollider& b) { b = snapshot; },
                                              UndoHistory::randomGroupId(label));
    }
};
