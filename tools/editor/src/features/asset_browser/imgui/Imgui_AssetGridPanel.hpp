#pragma once
#include <filesystem>
#include <imgui.h>
#include <string>
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetQuickActions.hpp"
#include "../../../services/common_editing/AssetStore.hpp"
#include "../../../styling/EditorPanel.hpp"
#include "../../../styling/ImguiStyling.hpp"
#include "../../../styling/SearchBar.hpp"
#include "Imgui_AssetDropdownMenu.hpp"
#include "modules/rendering/vulkan/core/resources/AssetThumbnailGenerator.hpp"
#include "modules/rendering/vulkan/core/resources/ImguiThumbnail.hpp"

class Imgui_AssetGridPanel : public EditorPanel {
public:
    static constexpr int SEARCH_BUFFER_SIZE = 256;

    Imgui_AssetGridPanel(AssetStore& assetStore,
                         EditorSelection& editorSelection,
                         AssetQuickActions& assetQuickActions,
                         AssetThumbnailGenerator& assetThumbnailGenerator) :
        assetStore_(assetStore),
        editorSelection_(editorSelection),
        assetThumbnailGenerator_(assetThumbnailGenerator),
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
        const char* ext1; // primary extension (nullptr = show all)
        const char* ext2; // optional second extension (e.g. .jpg alongside .png)
    };

    static constexpr Tab TABS[] = {
            {"All", nullptr, nullptr},
            {"Materials", ".mat", nullptr},
            {"Meshes", ".mesh", nullptr},
            {"Models", ".model", nullptr},
            {"Textures", ".png", ".jpg"},
            {"Shaders", ".shad", nullptr},
            {"Scripts", ".lua", nullptr},
            {"Animators", ".animctrl", nullptr},
            {"Prefabs", ".prefab", nullptr},
    };

    static constexpr std::string_view knownExtensions[] = {
            ".mesh",
            ".mat",
            ".shad",
            ".png",
            ".jpg",
            ".model",
            ".lua",
            ".animctrl",
            ".prefab",
    };

    AssetStore& assetStore_;
    EditorSelection& editorSelection_;
    AssetThumbnailGenerator& assetThumbnailGenerator_;
    Imgui_AssetDropdownMenu dropdownMenu_;
    std::optional<std::string> contextTargetAsset_;
    char searchBuffer_[SEARCH_BUFFER_SIZE];
    int activeTab_ = 0;

    static bool isKnownAsset(const std::string& path) {
        auto ext = std::filesystem::path(path).extension().string();
        for (auto& known: knownExtensions) {
            if (ext == known) return true;
        }
        return false;
    }

    bool matchesSearch(const std::string& asset) const {
        if (searchBuffer_[0] == '\0') return true;
        std::string lower = asset;
        std::string query = searchBuffer_;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
        return lower.find(query) != std::string::npos;
    }

    bool matchesTab(const std::string& asset, const Tab& tab) const {
        if (tab.ext1 == nullptr) return true;
        auto ext = std::filesystem::path(asset).extension().string();
        if (ext == tab.ext1) return true;
        if (tab.ext2 != nullptr && ext == tab.ext2) return true;
        return false;
    }

    void drawSearchBar() { ::drawSearchBar("AssetGridSearch", searchBuffer_, SEARCH_BUFFER_SIZE); }

    void drawTabs() {
        if (!ImGui::BeginTabBar("##AssetTabs")) return;

        for (int i = 0; i < static_cast<int>(std::size(TABS)); ++i) {
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
        std::vector<std::string> assets;
        for (auto& file: assetStore_.listAll()) {
            if (isKnownAsset(file) && matchesSearch(file) && matchesTab(file, tab)) assets.push_back(file);
        }
        std::ranges::sort(assets);

        constexpr float thumbSize = 64.0f;
        constexpr float padding = 2.0f;
        constexpr float cellSize = thumbSize + padding * 2.0f;

        int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cellSize));

        bool cmdDown = ImGui::GetIO().KeySuper || ImGui::GetIO().KeyCtrl;
        bool itemRightClicked = false;
        int col = 0;
        float originX = ImGui::GetCursorPosX();
        float rowY = ImGui::GetCursorPosY();

        for (const auto& asset: assets) {
            ImGui::SetCursorPosX(originX + static_cast<float>(col) * cellSize);
            ImGui::SetCursorPosY(rowY);

            ImGui::BeginGroup();

            ImguiThumbnail thumbnail = assetThumbnailGenerator_.get(asset);
            float aspect =
                    thumbnail ? static_cast<float>(thumbnail.width) / static_cast<float>(thumbnail.height) : 1.0f;
            ImVec2 imgSize =
                    aspect >= 1.0f ? ImVec2(thumbSize, thumbSize / aspect) : ImVec2(thumbSize * aspect, thumbSize);

            bool selected = editorSelection_.isAssetSelected(asset);
            if (selected) {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(
                        pos, {pos.x + cellSize, pos.y + cellSize}, IM_COL32(70, 120, 200, 80));
            }

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding);

            if (thumbnail) {
                ImGui::Image((ImTextureID) thumbnail.descriptorSet,
                             imgSize,
                             {0, 0},
                             {1, 1},
                             {1, 1, 1, 1},
                             {0.4f, 0.4f, 0.4f, 1.0f});
            } else {
                ImGui::Dummy(imgSize);
            }

            if (asset.ends_with(".mat") && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("MATERIAL_ASSET", asset.c_str(), asset.size() + 1);
                ImGui::TextUnformatted(asset.c_str());
                ImGui::EndDragDropSource();
            }
            if ((asset.ends_with(".png") || asset.ends_with(".jpg")) &&
                ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("texture_asset", asset.c_str(), asset.size() + 1);
                ImGui::TextUnformatted(asset.c_str());
                ImGui::EndDragDropSource();
            }
            if (asset.ends_with(".lua") && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("SCRIPT_ASSET", asset.c_str(), asset.size() + 1);
                ImGui::TextUnformatted(asset.c_str());
                ImGui::EndDragDropSource();
            }
            if (asset.ends_with(".mesh") && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("MESH_ASSET", asset.c_str(), asset.size() + 1);
                ImGui::TextUnformatted(asset.c_str());
                ImGui::EndDragDropSource();
            }
            if (asset.ends_with(".animctrl") && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("ANIMCTRL_ASSET", asset.c_str(), asset.size() + 1);
                ImGui::TextUnformatted(asset.c_str());
                ImGui::EndDragDropSource();
            }
            if (asset.ends_with(".prefab") && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("PREFAB_ASSET", asset.c_str(), asset.size() + 1);
                ImGui::TextUnformatted(asset.c_str());
                ImGui::EndDragDropSource();
            }

            std::string filename = std::filesystem::path(asset).filename().string();
            constexpr std::string_view ellipsis = "...";
            float ellipsisWidth = ImGui::CalcTextSize(ellipsis.data()).x;
            std::string displayName = filename;
            if (ImGui::CalcTextSize(displayName.c_str()).x > cellSize) {
                while (!displayName.empty() && ImGui::CalcTextSize(displayName.c_str()).x + ellipsisWidth > cellSize)
                    displayName.pop_back();
                displayName += ellipsis;
            }
            ImGui::SetWindowFontScale(0.8f);
            float textOffsetX = std::max(0.0f, (cellSize - ImGui::CalcTextSize(displayName.c_str()).x) / 2.0f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffsetX);
            ImGui::TextUnformatted(displayName.c_str());
            ImGui::SetWindowFontScale(1.0f);

            ImGui::EndGroup();

            bool hovered = ImGui::IsItemHovered();
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (cmdDown && selected)
                    editorSelection_.removeFromSelection(asset);
                else
                    editorSelection_.addToSelection(asset, cmdDown);
            }
            if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                editorSelection_.setInspectedAsset(asset);
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                contextTargetAsset_.emplace(asset);
                itemRightClicked = true;
            }

            col = (col + 1) % columns;
            if (col == 0) rowY = ImGui::GetCursorPosY();
        }

        bool windowHovered =
                ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        if (windowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (!itemRightClicked) contextTargetAsset_.reset();
            ImGui::OpenPopup("AssetGridContextMenu");
        }

        dropdownMenu_.draw(contextTargetAsset_, "", "AssetGridContextMenu", editorSelection_.getSelectedAssetIds());
    }
};
