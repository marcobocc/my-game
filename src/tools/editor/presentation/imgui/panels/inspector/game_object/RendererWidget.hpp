#pragma once
#include <glm/glm.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include <unordered_set>
#include "../../../../../business/EditorGizmos.hpp"
#include "../../../../../business/scene_editing/SceneMutations.hpp"
#include "../ComponentContainer.hpp"
#include "data/assets/Material.hpp"
#include "data/components/Renderer.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/scene/EntityManager.hpp"

class RendererWidget : public ComponentContainer {
public:
    RendererWidget(AssetManager& assetManager, SceneMutations& sceneMutations, EditorGizmos& debugViz) :
        ComponentContainer("Renderer", 8),
        assetManager_(assetManager),
        sceneMutations_(sceneMutations),
        debugViz_(debugViz) {}

    void setComponent(Renderer& r, EntityHandle objectId) {
        component_ = &r;
        objectId_ = objectId;
    }

private:
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    EditorGizmos& debugViz_;
    Renderer* component_ = nullptr;
    EntityHandle objectId_{};

    void drawBody() override {
        if (!component_) return;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

        row("Mesh", [&] { ImGui::TextUnformatted(component_->meshName.c_str()); });
        row("Material", [&] { ImGui::TextUnformatted(component_->materialName.c_str()); });

        if (const Material* mat = assetManager_.get<Material>(component_->materialName)) {
            row("Texture", [&] {
                if (!mat->getTextureName().empty())
                    ImGui::TextUnformatted(mat->getTextureName().c_str());
                else
                    ImGui::TextDisabled("none");
            });

            glm::vec4 color = component_->baseColorOverride.value_or(mat->getBaseColor());
            float col[4] = {color.r, color.g, color.b, color.a};
            if (ImGui::ColorEdit4("Base color", col)) {
                component_->baseColorOverride = glm::vec4(col[0], col[1], col[2], col[3]);
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
