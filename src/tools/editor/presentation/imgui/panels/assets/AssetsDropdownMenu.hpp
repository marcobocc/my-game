#pragma once
#include <filesystem>
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../../business/EditorSelection.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/asset_editing/MaterialMutations.hpp"

class AssetsDropdownMenu {
public:
    AssetsDropdownMenu(EditorSelection& editorSelection,
                       EditorAssetRepository& repository,
                       MaterialMutations& materialMutations) :
        editorSelection_(editorSelection),
        repository_(repository),
        materialMutations_(materialMutations) {}

    void draw(const std::optional<std::string>& selectedAsset) {
        if (selectedAsset) {
            bool isMutable = materialMutations_.isMutable(*selectedAsset);
            if (!isMutable) {
                ImGui::BeginDisabled();
            }
            if (ImGui::MenuItem("Delete")) {
                deleteMaterial(*selectedAsset);
            }
            if (!isMutable) {
                ImGui::EndDisabled();
            }
            ImGui::Separator();
        }
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Material")) {
                materialMutations_.createNew();
            }
            ImGui::EndMenu();
        }
    }

private:
    EditorSelection& editorSelection_;
    EditorAssetRepository& repository_;
    MaterialMutations& materialMutations_;

    void deleteMaterial(const std::string& assetName) {
        if (assetName.ends_with(".mat")) {
            repository_.remove<Material>(assetName);
            if (editorSelection_.isAssetSelected(assetName)) {
                editorSelection_.clearSelection();
            }
        }
    }
};
