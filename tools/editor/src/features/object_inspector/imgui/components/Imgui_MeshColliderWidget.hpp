#pragma once
#include <optional>
#include <string>
#include "../../../../../../../runtime/src/physics/components/MeshCollider.hpp"
#include "../../../../features/asset_browser/imgui/Imgui_AssetPicker.hpp"
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../../../scene_viewport/gizmos/EditorGizmos.hpp"
#include "../Imgui_InspectorWidget.hpp"

// Shows the mesh collider's derived state (it has no directly editable shape — the
// hull is always recomputed from the source mesh). The only real setting is an
// optional mesh override; left empty, it follows the entity's Renderer mesh.
class Imgui_MeshColliderWidget : public Imgui_InspectorWidget {
public:
    Imgui_MeshColliderWidget(RuntimeScene& scene, EditorGizmos& debugViz, Imgui_AssetPicker& assetPicker) :
        Imgui_InspectorWidget("Mesh Collider"),
        scene_(scene),
        debugViz_(debugViz),
        assetPicker_(assetPicker) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const MeshCollider& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    EditorGizmos& debugViz_;
    Imgui_AssetPicker& assetPicker_;
    std::optional<EntityHandle> lastObjectId_;
    MeshCollider component_{};

    void applyMeshOverride(const std::string& name) {
        component_.meshName = name;
        component_.hullDirty = true;
        MeshCollider snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<MeshCollider>([snapshot](MeshCollider& c) { c = snapshot; },
                                               UndoHistory::randomGroupId("Set Mesh Override"));
    }

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<CheckboxProperty>(
                "Show Collider",
                [this] { return debugViz_.meshColliderEnabled(*lastObjectId_); },
                [this](bool) { debugViz_.toggleMeshCollider(*lastObjectId_); });

        add<InputTextProperty>(
                "Mesh Override",
                [this] { return component_.meshName; },
                "MESH_ASSET",
                [this](const char* name) { applyMeshOverride(name); },
                [this] {
                    assetPicker_.open(
                            "Select Mesh", {".mesh"}, [this](const std::string& name) { applyMeshOverride(name); });
                });

        ImGui::TextDisabled(component_.meshName.empty() ? "Following entity's Renderer mesh." : "Using mesh override.");
        if (!component_.hull.vertices.empty()) {
            ImGui::Text("Hull: %zu verts, %zu faces", component_.hull.vertices.size(), component_.hull.faces.size());
        } else {
            ImGui::TextDisabled("Hull not built yet (enable \"Show Collider\" to build it).");
        }
    }
};
