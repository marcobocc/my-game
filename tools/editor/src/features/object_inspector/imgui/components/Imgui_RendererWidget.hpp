#pragma once
#include <atomic>
#include <glm/glm.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include <unordered_set>
#include "../../../../services/common_editing/AssetStore.hpp"
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../../../scene_viewport/gizmos/EditorGizmos.hpp"
#include "../Imgui_InspectorWidget.hpp"
#include "modules/asset_management/asset_types/Material.hpp"
#include "modules/scene/components/Renderer.hpp"

class Imgui_RendererWidget : public Imgui_InspectorWidget {
public:
    Imgui_RendererWidget(AssetStore& assetStore, RuntimeScene& scene, EditorGizmos& debugViz) :
        Imgui_InspectorWidget("Renderer", 8),
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

    void drawBody() override {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

        row("Mesh", [&] { ImGui::TextUnformatted(component_.meshName.c_str()); });
        row("Material", [&] {
            ImGui::TextUnformatted(component_.materialName.c_str());

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_ASSET")) {
                    const char* materialName = static_cast<const char*>(payload->Data);
                    component_.materialName = materialName;
                    Renderer snapshot = component_;
                    scene_.getObject(objectId_).mutateComponent<Renderer>([snapshot](Renderer& r) { r = snapshot; },
                                                                          UndoHistory::randomGroupId("Apply Material"));
                }
                ImGui::EndDragDropTarget();
            }
        });

        try {
            const Material& mat = assetStore_.getAsset<Material>(component_.materialName);
            glm::vec4 color = component_.tintOverride.value_or(mat.tint);
            float col[4] = {color.r, color.g, color.b, color.a};
            if (ImGui::ColorEdit4("Tint", col)) {
                component_.tintOverride = glm::vec4(col[0], col[1], col[2], col[3]);
            }
            if (ImGui::IsItemActivated()) groupId_ = UndoHistory::randomGroupId("Set Tint");
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Renderer snapshot = component_;
                scene_.getObject(objectId_).mutateComponent<Renderer>([snapshot](Renderer& r) { r = snapshot; },
                                                                      groupId_);
                groupId_.clear();
            }
        } catch (const std::exception&) {
            // Material doesn't exist
        }

        checkboxRow("Show Bounding Box", debugViz_.getObjectsWithAABBEnabled().contains(objectId_), [&] {
            debugViz_.toggleAABB(objectId_);
        });
        checkboxRow("Show Bounding Sphere", debugViz_.getObjectsWithBoundingSpheresEnabled().contains(objectId_), [&] {
            debugViz_.toggleBoundingSphere(objectId_);
        });

        ImGui::PopStyleVar();
    }

    template<typename Fn>
    static void row(const char* label, Fn&& fn) {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 90.0f);
        ImGui::Text("%s:", label);
        ImGui::NextColumn();
        fn();
        ImGui::Columns(1);
    }

    template<typename Fn>
    static void checkboxRow(const char* label, bool value, Fn&& onToggle) {
        bool v = value;
        ImGui::PushID(label);
        if (ImGui::Checkbox("##cb", &v)) onToggle();
        ImGui::SameLine();
        ImGui::TextUnformatted(label);
        ImGui::PopID();
    }
};
