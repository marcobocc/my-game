#pragma once
#include <optional>
#include <string>
#include "../../../../../../../runtime/src/graphics/assets/Material.hpp"
#include "../../../../../../../runtime/src/graphics/components/Renderer.hpp"
#include "../../../../features/asset_browser/imgui/Imgui_AssetPicker.hpp"
#include "../../../../services/common_editing/AssetStore.hpp"
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../../../scene_viewport/gizmos/EditorGizmos.hpp"
#include "../Imgui_InspectorWidget.hpp"

class Imgui_RendererWidget : public Imgui_InspectorWidget {
public:
    Imgui_RendererWidget(AssetStore& assetStore,
                         RuntimeScene& scene,
                         EditorGizmos& debugViz,
                         Imgui_AssetPicker& assetPicker) :
        Imgui_InspectorWidget("Renderer"),
        assetStore_(assetStore),
        scene_(scene),
        debugViz_(debugViz),
        assetPicker_(assetPicker) {}

    void setComponent(const Renderer& r, EntityHandle objectId) {
        component_ = r;
        objectId_ = objectId;
    }

private:
    AssetStore& assetStore_;
    RuntimeScene& scene_;
    EditorGizmos& debugViz_;
    Imgui_AssetPicker& assetPicker_;
    Renderer component_{};
    EntityHandle objectId_{};
    std::string groupId_;

    void applyMesh(const std::string& name) {
        component_.meshName = name;
        Renderer snapshot = component_;
        scene_.getObject(objectId_).mutateComponent<Renderer>([snapshot](Renderer& r) { r = snapshot; },
                                                              UndoHistory::randomGroupId("Apply Mesh"));
    }

    void applyMaterial(const std::string& name) {
        component_.materialName = name;
        Renderer snapshot = component_;
        scene_.getObject(objectId_).mutateComponent<Renderer>([snapshot](Renderer& r) { r = snapshot; },
                                                              UndoHistory::randomGroupId("Apply Material"));
    }

    void buildProperties() override {
        add<InputTextProperty>(
                "Mesh",
                [this] { return component_.meshName; },
                "MESH_ASSET",
                [this](const char* name) { applyMesh(name); },
                [this] {
                    assetPicker_.open("Select Mesh", {".mesh"}, [this](const std::string& name) { applyMesh(name); });
                });

        add<InputTextProperty>(
                "Material",
                [this] { return component_.materialName; },
                "MATERIAL_ASSET",
                [this](const char* name) { applyMaterial(name); },
                [this] {
                    assetPicker_.open("Select Material", {".mat", ".matpkg"}, [this](const std::string& name) {
                        applyMaterial(name);
                    });
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
