#pragma once
#include <glm/glm.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include <unordered_set>
#include "../../../../../business/EditorGizmos.hpp"
#include "../../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../../business/scene_editing/SceneMutations.hpp"
#include "../ComponentContainer.hpp"
#include "modules/asset_management/asset_types/Material.hpp"
#include "modules/scene/EntityManager.hpp"
#include "modules/scene/components/Renderer.hpp"

class RendererWidget : public ComponentContainer {
public:
    RendererWidget(EditorAssetRepository& assetRepository, SceneMutations& sceneMutations, EditorGizmos& debugViz) :
        ComponentContainer("Renderer", 8),
        assetRepository_(assetRepository),
        sceneMutations_(sceneMutations),
        debugViz_(debugViz) {}

    void setComponent(Renderer& r, EntityHandle objectId) {
        component_ = &r;
        objectId_ = objectId;
    }

private:
    EditorAssetRepository& assetRepository_;
    SceneMutations& sceneMutations_;
    EditorGizmos& debugViz_;
    Renderer* component_ = nullptr;
    EntityHandle objectId_{};

    void drawBody() override {
        if (!component_) return;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

        row("Mesh", [&] { ImGui::TextUnformatted(component_->meshName.c_str()); });
        row("Material", [&] {
            ImGui::TextUnformatted(component_->materialName.c_str());

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_ASSET")) {
                    const char* materialName = static_cast<const char*>(payload->Data);
                    component_->materialName = materialName;
                    sceneMutations_.commitEdit(objectId_);
                }
                ImGui::EndDragDropTarget();
            }
        });

        if (const Material* mat = assetRepository_.get<Material>(component_->materialName)) {
            glm::vec4 color = component_->tintOverride.value_or(mat->tint);
            float col[4] = {color.r, color.g, color.b, color.a};
            if (ImGui::ColorEdit4("Tint", col)) {
                component_->tintOverride = glm::vec4(col[0], col[1], col[2], col[3]);
            }
            if (ImGui::IsItemActivated()) sceneMutations_.beginEdit(objectId_);
            if (ImGui::IsItemDeactivatedAfterEdit()) sceneMutations_.commitEdit(objectId_);
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
        if (ImGui::Checkbox("##cb", &v)) {
            onToggle();
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(label);
        ImGui::PopID();
    }
};
