#pragma once
#include <filesystem>
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../../business/asset_editing/MaterialMutations.hpp"

class AssetsDropdownMenu {
public:
    AssetsDropdownMenu(ObjectSelection& objectSelection, MaterialMutations& materialMutations) :
        objectSelection_(objectSelection),
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
    ObjectSelection& objectSelection_;
    MaterialMutations& materialMutations_;

    void deleteMaterial(const std::string& assetName) {
        if (assetName.ends_with(".mat")) {
            materialMutations_.deleteMaterial(assetName);
        }
    }
};
