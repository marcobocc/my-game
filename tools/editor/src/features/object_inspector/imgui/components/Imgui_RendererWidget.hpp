#pragma once
#include <optional>
#include <string>
#include "../../../../services/common_editing/AssetStore.hpp"
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../../../scene_viewport/gizmos/EditorGizmos.hpp"
#include "../Imgui_InspectorWidget.hpp"
#include "modules/asset_management/asset_types/Material.hpp"
#include "modules/scene/components/Renderer.hpp"

class Imgui_RendererWidget : public Imgui_InspectorWidget {
public:
    Imgui_RendererWidget(AssetStore& assetStore, RuntimeScene& scene, EditorGizmos& debugViz) :
        Imgui_InspectorWidget("Renderer"),
        assetStore_(assetStore),
        scene_(scene),
        debugViz_(debugViz) {}

    void setComponent(const Renderer& r, EntityHandle objectId) {
        component_ = r;
        objectId_ = objectId;
    }

private:
    AssetStore& assetStore_;
    RuntimeScene& scene_;
    EditorGizmos& debugViz_;
    Renderer component_{};
    EntityHandle objectId_{};
    std::string groupId_;

    void buildProperties() override {
        add<InputTextProperty>(
                "Mesh",
                [this] { return component_.meshName; },
                "MESH_ASSET",
                [this](const char* name) {
                    component_.meshName = name;
                    Renderer snapshot = component_;
                    scene_.getObject(objectId_).mutateComponent<Renderer>([snapshot](Renderer& r) { r = snapshot; },
                                                                          UndoHistory::randomGroupId("Apply Mesh"));
                });

        add<InputTextProperty>(
                "Material",
                [this] { return component_.materialName; },
                "MATERIAL_ASSET",
                [this](const char* name) {
                    component_.materialName = name;
                    Renderer snapshot = component_;
                    scene_.getObject(objectId_).mutateComponent<Renderer>([snapshot](Renderer& r) { r = snapshot; },
                                                                          UndoHistory::randomGroupId("Apply Material"));
                });

        try {
            const Material& mat = assetStore_.getAsset<Material>(component_.materialName);
            glm::vec4 tint = component_.tintOverride.value_or(mat.tint);
            add<ColorEdit4Property>(
                    "Tint",
                    [this, tint] {
                        try {
                            const Material& m = assetStore_.getAsset<Material>(component_.materialName);
                            return component_.tintOverride.value_or(m.tint);
                        } catch (...) {
                            return tint;
                        }
                    },
                    [this](glm::vec4 v) { component_.tintOverride = v; },
                    [this] { groupId_ = UndoHistory::randomGroupId("Set Tint"); },
                    [this] {
                        Renderer snapshot = component_;
                        scene_.getObject(objectId_).mutateComponent<Renderer>([snapshot](Renderer& r) { r = snapshot; },
                                                                              groupId_);
                        groupId_.clear();
                    });
        } catch (...) {
        }

        add<CheckboxProperty>(
                "Show Bounding Box",
                [this] { return debugViz_.getObjectsWithAABBEnabled().contains(objectId_); },
                [this](bool) { debugViz_.toggleAABB(objectId_); });

        add<CheckboxProperty>(
                "Show Bounding Sphere",
                [this] { return debugViz_.getObjectsWithBoundingSpheresEnabled().contains(objectId_); },
                [this](bool) { debugViz_.toggleBoundingSphere(objectId_); });
    }
};
