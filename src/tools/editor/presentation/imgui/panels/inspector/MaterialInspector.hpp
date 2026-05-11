#pragma once
#include <glm/glm.hpp>
#include <imgui.h>
#include "../../../../business/ObjectSelection.hpp"
#include "ComponentContainer.hpp"
#include "data/assets/Material.hpp"
#include "modules/assets/AssetManager.hpp"

class MaterialInspector : public ComponentContainer {
public:
    MaterialInspector(AssetManager& assetManager, ObjectSelection& objectSelection) :
        ComponentContainer("Material", 6),
        assetManager_(assetManager),
        objectSelection_(objectSelection) {}

private:
    AssetManager& assetManager_;
    ObjectSelection& objectSelection_;

    void drawBody() override {
        auto selectedAsset = objectSelection_.getSelectedAssetId();
        if (!selectedAsset) return;

        if (const Material* mat = assetManager_.get<Material>(*selectedAsset)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

            row("Name", [&] { ImGui::TextUnformatted(selectedAsset->c_str()); });
            row("Texture", [&] {
                if (!mat->getTextureName().empty())
                    ImGui::TextUnformatted(mat->getTextureName().c_str());
                else
                    ImGui::TextDisabled("none");
            });

            glm::vec4 color = mat->getBaseColor();
            float col[4] = {color.r, color.g, color.b, color.a};
            if (ImGui::ColorEdit4("Base color", col)) {
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
