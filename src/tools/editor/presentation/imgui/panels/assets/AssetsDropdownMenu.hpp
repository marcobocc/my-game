#pragma once
#include <filesystem>
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../../business/ObjectSelection.hpp"
#include "../../../../business/asset_editing/MaterialMutations.hpp"
#include "modules/assets/AssetManager.hpp"

class AssetsDropdownMenu {
public:
    AssetsDropdownMenu(AssetManager& assetManager,
                       ObjectSelection& objectSelection,
                       MaterialMutations& materialMutations) :
        assetManager_(assetManager),
        objectSelection_(objectSelection),
        materialMutations_(materialMutations) {}

    void draw(std::optional<std::string> selectedAsset) {
        if (selectedAsset) {
            bool isBuiltin = materialMutations_.isBuiltin(*selectedAsset);
            if (isBuiltin) {
                ImGui::BeginDisabled();
            }
            if (ImGui::MenuItem("Delete")) {
                deleteMaterial(*selectedAsset);
            }
            if (isBuiltin) {
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
    AssetManager& assetManager_;
    ObjectSelection& objectSelection_;
    MaterialMutations& materialMutations_;

    void deleteMaterial(const std::string& assetName) {
        if (assetName.ends_with(".mat")) {
            materialMutations_.deleteMaterial(assetName);
        }
    }
};
