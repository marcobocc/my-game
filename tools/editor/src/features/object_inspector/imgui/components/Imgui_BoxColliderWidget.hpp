#pragma once
#include <optional>
#include <string>
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../../../scene_viewport/gizmos/EditorGizmos.hpp"
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/BoxCollider.hpp"

class Imgui_BoxColliderWidget : public Imgui_InspectorWidget {
public:
    Imgui_BoxColliderWidget(RuntimeScene& scene, EditorGizmos& debugViz) :
        Imgui_InspectorWidget("Box Collider"),
        scene_(scene),
        debugViz_(debugViz) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const BoxCollider& b) { component_ = b; }

private:
    RuntimeScene& scene_;
    EditorGizmos& debugViz_;
    std::optional<EntityHandle> lastObjectId_;
    BoxCollider component_{};
    std::string groupId_;

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<CheckboxProperty>(
                "Show Collider",
                [this] { return debugViz_.boxColliderEnabled(*lastObjectId_); },
                [this](bool) { debugViz_.toggleBoxCollider(*lastObjectId_); });

        add<Vec3Property>(
                "Center",
                [this] { return component_.center; },
                [this](glm::vec3 v) { component_.center = v; },
                0.01f,
                [this] { groupId_ = UndoHistory::randomGroupId("Set Center"); },
                [this] { commit(); });

        add<Vec3Property>(
                "Half Extents",
                [this] { return component_.halfExtents; },
                [this](glm::vec3 v) { component_.halfExtents = v; },
                0.01f,
                [this] { groupId_ = UndoHistory::randomGroupId("Set Half Extents"); },
                [this] { commit(); });
    }

    void commit() {
        if (!lastObjectId_) return;
        BoxCollider snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<BoxCollider>([snapshot](BoxCollider& b) { b = snapshot; }, groupId_);
        groupId_.clear();
    }
};
