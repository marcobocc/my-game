#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include <vector>
#include "../../../../business/EditorSelection.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/asset_editing/MaterialMutations.hpp"
#include "../../../components/DropdownMenuComponent.hpp"

class AssetsDropdownMenu : public DropdownMenuComponent {
public:
    AssetsDropdownMenu(EditorSelection& editorSelection,
                       EditorAssetRepository& repository,
                       MaterialMutations& materialMutations) :
        editorSelection_(editorSelection),
        repository_(repository),
        materialMutations_(materialMutations) {}

    void draw(const std::optional<std::string>& contextAsset,
              const std::string& selectedFolder,
              const char* popupId,
              const std::vector<std::string>& explorerSelection = {}) {
        contextAsset_ = contextAsset;
        selectedFolder_ = selectedFolder;
        explorerSelection_ = explorerSelection;

        std::vector<std::string> deleteTargets;
        if (contextAsset_.has_value()) {
            deleteTargets = {*contextAsset_};
        } else {
            deleteTargets = explorerSelection_;
        }

        bool anyDeletable = std::ranges::any_of(deleteTargets,
                                                [&](const std::string& a) { return materialMutations_.isMutable(a); });

        setMenuItems({{.label = "Delete", .enabled = anyDeletable, .onItemClick = [this, deleteTargets] {
                           deleteSelection();
                       }}});
        DropdownMenuComponent::draw(popupId);
    }

    void deleteSelection() { deleteMaterials(editorSelection_.getSelectedAssetIds()); }

protected:
    void drawBody() override {
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Material")) {
                materialMutations_.createNew(selectedFolder_);
            }
            ImGui::EndMenu();
        }
    }

private:
    EditorSelection& editorSelection_;
    EditorAssetRepository& repository_;
    MaterialMutations& materialMutations_;
    std::optional<std::string> contextAsset_;
    std::string selectedFolder_;
    std::vector<std::string> explorerSelection_;

    void deleteMaterials(const std::vector<std::string>& assetNames) {
        std::vector<std::string> mats;
        for (const auto& name: assetNames) {
            if (name.ends_with(".mat")) mats.push_back(name);
        }
        if (mats.empty()) return;
        repository_.removeMany<Material>(mats);
        editorSelection_.clearSelection();
    }
};
