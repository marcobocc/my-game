#pragma once
#include <glm/glm.hpp>
#include <imgui.h>
#include "../../../../business/ObjectSelection.hpp"
#include "../../../../business/asset_editing/MaterialMutations.hpp"
#include "ComponentContainer.hpp"
#include "modules/assets/AssetManager.hpp"
#include "structs/assets/Material.hpp"

class MaterialInspector : public ComponentContainer {
public:
    MaterialInspector(AssetManager& assetManager,
                      ObjectSelection& objectSelection,
                      MaterialMutations& materialMutations) :
        ComponentContainer("Material", 6),
        assetManager_(assetManager),
        objectSelection_(objectSelection),
        materialMutations_(materialMutations) {}

private:
    AssetManager& assetManager_;
    ObjectSelection& objectSelection_;
    MaterialMutations& materialMutations_;

    void drawBody() override {
        auto selectedAsset = objectSelection_.getSelectedAssetId();
        if (!selectedAsset) return;

        if (const Material* mat = assetManager_.get<Material>(*selectedAsset)) {
            bool isBuiltin = materialMutations_.isBuiltin(*selectedAsset);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

            if (isBuiltin) {
                ImGui::BeginDisabled();
            }

            row("Name", [&] { ImGui::TextUnformatted(selectedAsset->c_str()); });
            row("Albedo", [&] {
                if (!mat->getAlbedoTexture().empty())
                    ImGui::TextUnformatted(mat->getAlbedoTexture().c_str());
                else
                    ImGui::TextDisabled("none");
            });

            glm::vec4 color = mat->getBaseColor();
            float col[4] = {color.r, color.g, color.b, color.a};
            if (ImGui::ColorEdit4("Base color", col)) {
                materialMutations_.setBaseColor(*selectedAsset, glm::vec4(col[0], col[1], col[2], col[3]));
            }

            if (isBuiltin) {
                ImGui::EndDisabled();
                ImGui::Spacing();
                ImGui::TextDisabled("(Builtin materials cannot be edited)");
            }

            ImGui::PopStyleVar();
        }
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
};
