#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include <vector>
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetQuickActions.hpp"
#include "../../../services/common_editing/AssetStore.hpp"
#include "../../../styling/DropdownMenuComponent.hpp"

class Imgui_AssetDropdownMenu : public DropdownMenuComponent {
public:
    Imgui_AssetDropdownMenu(EditorSelection& editorSelection,
                            AssetStore& assetStore,
                            AssetQuickActions& assetQuickActions) :
        editorSelection_(editorSelection),
        assetStore_(assetStore),
        assetQuickActions_(assetQuickActions) {}

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

        bool anyDeletable =
                std::ranges::any_of(deleteTargets, [&](const std::string& a) { return assetStore_.isMutable(a); });

        setMenuItems({{.label = "Delete", .enabled = anyDeletable, .onItemClick = [this, deleteTargets] {
                           editorSelection_.clearSelection();
                           for (const auto& asset: deleteTargets) {
                               editorSelection_.addToSelection(asset, true);
                           }
                           assetQuickActions_.deleteSelection();
                       }}});
        DropdownMenuComponent::draw(popupId);
    }

protected:
    void drawBody() override {
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Material")) {
                assetQuickActions_.createMaterial(selectedFolder_);
            }
            ImGui::EndMenu();
        }
    }

private:
    EditorSelection& editorSelection_;
    AssetStore& assetStore_;
    AssetQuickActions& assetQuickActions_;
    std::optional<std::string> contextAsset_;
    std::string selectedFolder_;
    std::vector<std::string> explorerSelection_;
};
