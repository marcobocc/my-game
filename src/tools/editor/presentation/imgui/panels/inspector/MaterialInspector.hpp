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
        materialMutations_(materialMutations),
        colorPickerValue_{1.0f, 1.0f, 1.0f, 1.0f} {}

private:
    AssetManager& assetManager_;
    ObjectSelection& objectSelection_;
    MaterialMutations& materialMutations_;
    float colorPickerValue_[4];

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
                ImGui::SetNextItemWidth(-1);
                const std::string displayText =
                        (mat->getAlbedoTexture() == EMPTY_TEXTURE) ? "" : mat->getAlbedoTexture();
                ImGui::InputText("##albedoTexture",
                                 const_cast<char*>(displayText.c_str()),
                                 displayText.size() + 1,
                                 ImGuiInputTextFlags_ReadOnly);
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_asset")) {
                        const char* textureName = static_cast<const char*>(payload->Data);
                        materialMutations_.setAlbedoTexture(*selectedAsset, textureName);
                    }
                    ImGui::EndDragDropTarget();
                }
            });

            glm::vec4 color = mat->getTint();
            colorPickerValue_[0] = color.r;
            colorPickerValue_[1] = color.g;
            colorPickerValue_[2] = color.b;
            colorPickerValue_[3] = color.a;
            if (ImGui::ColorEdit4("Tint", colorPickerValue_)) {
                materialMutations_.setTint(*selectedAsset,
                                           glm::vec4(colorPickerValue_[0],
                                                     colorPickerValue_[1],
                                                     colorPickerValue_[2],
                                                     colorPickerValue_[3]));
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
