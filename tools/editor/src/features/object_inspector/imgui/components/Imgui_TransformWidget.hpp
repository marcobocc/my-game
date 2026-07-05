#pragma once
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/Metadata.hpp"
#include "modules/scene/components/Transform.hpp"

class RuntimeScene;

class Imgui_TransformWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_TransformWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Transform"), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const Transform& t) { component_ = t; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    Transform component_{};
    std::string groupId_;

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<LabelProperty>("Parent", [this] -> std::string {
            if (component_.parent == INVALID_ENTITY_HANDLE) return "(none)";
            const auto* meta = scene_.getObject(component_.parent).getComponent<Metadata>();
            return meta ? meta->displayName : std::to_string(component_.parent);
        });

        add<Vec3Property>(
                "Position",
                [this] { return component_.position; },
                [this](glm::vec3 v) { component_.position = v; },
                0.01f,
                [this] { groupId_ = UndoHistory::randomGroupId("Set Transform"); },
                [this] { commit(); });

        add<Vec3Property>(
                "Rotation",
                [this] { return glm::degrees(glm::eulerAngles(component_.rotation)); },
                [this](glm::vec3 euler) { component_.rotation = glm::quat(glm::radians(euler)); },
                0.5f,
                [this] { groupId_ = UndoHistory::randomGroupId("Set Transform"); },
                [this] { commit(); });

        add<Vec3Property>(
                "Scale",
                [this] { return component_.scale; },
                [this](glm::vec3 v) { component_.scale = v; },
                0.01f,
                [this] { groupId_ = UndoHistory::randomGroupId("Set Transform"); },
                [this] { commit(); });
    }

    void commit() {
        if (!lastObjectId_) return;
        Transform snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<Transform>([snapshot](Transform& t) { t = snapshot; }, groupId_);
        groupId_.clear();
    }
};
