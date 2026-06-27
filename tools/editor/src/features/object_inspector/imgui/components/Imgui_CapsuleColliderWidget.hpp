#pragma once
#include <optional>
#include <string>
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../../../scene_viewport/gizmos/EditorGizmos.hpp"
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/CapsuleCollider.hpp"

class Imgui_CapsuleColliderWidget : public Imgui_InspectorWidget {
public:
    Imgui_CapsuleColliderWidget(RuntimeScene& scene, EditorGizmos& debugViz) :
        Imgui_InspectorWidget("Capsule Collider"),
        scene_(scene),
        debugViz_(debugViz) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const CapsuleCollider& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    EditorGizmos& debugViz_;
    std::optional<EntityHandle> lastObjectId_;
    CapsuleCollider component_{};
    std::string groupId_;

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<CheckboxProperty>(
                "Show Collider",
                [this] { return debugViz_.capsuleColliderEnabled(*lastObjectId_); },
                [this](bool) { debugViz_.toggleCapsuleCollider(*lastObjectId_); });

        add<Vec3Property>(
                "Center",
                [this] { return component_.center; },
                [this](glm::vec3 v) { component_.center = v; },
                0.01f,
                [this] { groupId_ = UndoHistory::randomGroupId("Set Center"); },
                [this] { commit(); });

        add<DragFloatProperty>(
                "Radius",
                [this] { return component_.radius; },
                [this](float v) {
                    component_.radius = v;
                    groupId_ = UndoHistory::randomGroupId("Set Radius");
                    commit();
                },
                0.01f,
                0.0f,
                0.0f,
                [this] { commit(); });

        add<DragFloatProperty>(
                "Height",
                [this] { return component_.height; },
                [this](float v) {
                    component_.height = v;
                    groupId_ = UndoHistory::randomGroupId("Set Height");
                    commit();
                },
                0.01f,
                0.0f,
                0.0f,
                [this] { commit(); });
    }

    void commit() {
        if (!lastObjectId_) return;
        CapsuleCollider snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<CapsuleCollider>([snapshot](CapsuleCollider& c) { c = snapshot; }, groupId_);
        groupId_.clear();
    }
};
