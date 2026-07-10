#pragma once
#include <optional>
#include "../../../../../../../runtime/src/graphics/components/Camera.hpp"
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../Imgui_InspectorWidget.hpp"

class Imgui_CameraWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_CameraWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Camera"), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const Camera& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    Camera component_{};

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<CheckboxProperty>(
                "Active Camera",
                [this] { return scene_.isActiveCamera(*lastObjectId_); },
                [this](bool v) {
                    if (v)
                        scene_.setActiveCamera(*lastObjectId_);
                    else
                        scene_.clearActiveCamera();
                });
        add<SpacingProperty>();

        add<DragFloatProperty>(
                "FOV",
                [this] { return component_.fov; },
                [this](float v) { component_.fov = v; },
                0.5f,
                1.0f,
                179.0f,
                [this] { commit("Set FOV"); });

        add<DragFloatProperty>(
                "Near Plane",
                [this] { return component_.nearPlane; },
                [this](float v) { component_.nearPlane = v; },
                0.01f,
                0.001f,
                1000.0f,
                [this] { commit("Set Near Plane"); });

        add<DragFloatProperty>(
                "Far Plane",
                [this] { return component_.farPlane; },
                [this](float v) { component_.farPlane = v; },
                0.5f,
                0.1f,
                10000.0f,
                [this] { commit("Set Far Plane"); });

        add<SeparatorProperty>();
        add<FloatLabelProperty>("Aspect", [this] { return component_.aspect; }, "%.3f");
    }

    void commit(const char* label) {
        if (!lastObjectId_) return;
        Camera snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<Camera>([snapshot](Camera& c) { c = snapshot; }, UndoHistory::randomGroupId(label));
    }
};
