#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include <vector>
#include "../../../services/AssetThumbnailGenerator.hpp"
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetQuickActions.hpp"
#include "../../../services/common_editing/AssetStore.hpp"
#include "../../../styling/EditorPanel.hpp"
#include "../../../styling/SearchBar.hpp"
#include "Imgui_AssetDropdownMenu.hpp"
#include "Imgui_AssetGridView.hpp"

class Imgui_AssetGridPanel : public EditorPanel {
public:
    static constexpr int SEARCH_BUFFER_SIZE = 256;

    Imgui_AssetGridPanel(AssetStore& assetStore,
                         EditorSelection& editorSelection,
                         AssetQuickActions& assetQuickActions,
                         AssetThumbnailGenerator& assetThumbnailGenerator) :
        editorSelection_(editorSelection),
        gridView_(assetStore, assetThumbnailGenerator),
        dropdownMenu_(editorSelection, assetStore, assetQuickActions) {
        searchBuffer_[0] = '\0';
    }

    void draw() { EditorPanel::draw("Asset Grid"); }

    void drawBody() override {
        drawSearchBar();
        drawTabs();
    }

private:
    struct Tab {
        const char* label;
        std::vector<std::string> extensions; // empty = show all known extensions
    };

    inline static const std::vector<Tab> TABS = {
            {"All", {".model", ".modelpkg", ".prefab", ".mat", ".matpkg"}},
            {"Models", {".model", ".modelpkg"}},
            {"Prefabs", {".prefab"}},
            {"Materials", {".mat", ".matpkg"}},
    };

    EditorSelection& editorSelection_;
    Imgui_AssetGridView gridView_;
    Imgui_AssetDropdownMenu dropdownMenu_;
    std::optional<std::string> contextTargetAsset_;
    char searchBuffer_[SEARCH_BUFFER_SIZE];
    int activeTab_ = 0;

    static const char* dragPayloadType(const std::string& asset) {
        if (asset.ends_with(".mat") || asset.ends_with(".matpkg")) return "MATERIAL_ASSET";
        if (asset.ends_with(".model") || asset.ends_with(".modelpkg")) return "MODEL_ASSET";
        if (asset.ends_with(".prefab")) return "PREFAB_ASSET";
        return nullptr;
    }

    void drawSearchBar() { ::drawSearchBar("AssetGridSearch", searchBuffer_, SEARCH_BUFFER_SIZE); }

    void drawTabs() {
        if (!ImGui::BeginTabBar("##AssetTabs")) return;

        for (int i = 0; i < static_cast<int>(TABS.size()); ++i) {
            if (ImGui::BeginTabItem(TABS[i].label)) {
                activeTab_ = i;
                ImGui::BeginChild("##AssetGridScroll", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);
                drawAssetGrid(TABS[i]);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    void drawAssetGrid(const Tab& tab) {
        bool cmdDown = ImGui::GetIO().KeySuper || ImGui::GetIO().KeyCtrl;
        bool itemRightClicked = false;

        Imgui_AssetGridView::Callbacks cb;
        cb.isSelected = [this](const std::string& asset) { return editorSelection_.isAssetSelected(asset); };
        cb.onClick = [this, cmdDown](const std::string& asset) {
            if (cmdDown && editorSelection_.isAssetSelected(asset))
                editorSelection_.removeFromSelection(asset);
            else
                editorSelection_.addToSelection(asset, cmdDown);
        };
        cb.onDoubleClick = [this](const std::string& asset) { editorSelection_.setInspectedAsset(asset); };
        cb.onRightClick = [this, &itemRightClicked](const std::string& asset) {
            contextTargetAsset_.emplace(asset);
            itemRightClicked = true;
        };
        cb.dragPayloadType = [](const std::string& asset) { return dragPayloadType(asset); };

        gridView_.draw(gridView_.listAssets(tab.extensions, searchBuffer_), cb);

        bool windowHovered =
                ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        if (windowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (!itemRightClicked) contextTargetAsset_.reset();
            ImGui::OpenPopup("AssetGridContextMenu");
        }

        dropdownMenu_.draw(contextTargetAsset_, "", "AssetGridContextMenu", editorSelection_.getSelectedAssetIds());
    }
};
