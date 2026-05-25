#pragma once
#include <filesystem>
#include <imgui.h>
#include <string>
#include "../../../../../runtime/modules/rendering/vulkan/core/resources/AssetThumbnailGenerator.hpp"
#include "../../../../../runtime/modules/rendering/vulkan/core/resources/ImguiThumbnail.hpp"
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetQuickActions.hpp"
#include "../../../services/common_editing/AssetStore.hpp"
#include "../../../styling/EditorPanel.hpp"
#include "../../../styling/ImguiStyling.hpp"
#include "Imgui_AssetDropdownMenu.hpp"
#include "Imgui_FileExplorerComponent.hpp"

class AssetQuickActions;

class Imgui_AssetExplorerPanel : public EditorPanel {
public:
    static constexpr float ASSETS_HEIGHT_RATIO = 0.3f;
    static constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;
    static constexpr int SEARCH_BUFFER_SIZE = 256;

    Imgui_AssetExplorerPanel(AssetStore& assetStore,
                             EditorSelection& editorSelection,
                             AssetQuickActions& assetQuickActions,
                             AssetThumbnailGenerator& assetThumbnailGenerator) :
        assetStore_(assetStore),
        editorSelection_(editorSelection),
        assetQuickActions_(assetQuickActions),
        assetThumbnailGenerator_(assetThumbnailGenerator),
        dropdownMenu_(editorSelection, assetStore, assetQuickActions),
        fileExplorer_(
                {},
                [this](const std::string& item, bool isFolder, bool cmdDown) {
                    if (isFolder) {
                        selectedFolder_ = item;
                    } else {
                        selectedFolder_ = std::filesystem::path(item).parent_path().string();
                        if (cmdDown && editorSelection_.isAssetSelected(item)) {
                            editorSelection_.removeFromSelection(item);
                        } else {
                            editorSelection_.addToSelection(item, cmdDown);
                        }
                    }
                },
                [this](std::optional<std::string> item, bool isFolder) { contextTargetAsset_ = item; },
                [this](const std::string& item, bool isFolder) {
                    if (isFolder) {
                        return std::ranges::any_of(editorSelection_.getSelectedAssetIds(), [&](const std::string& s) {
                            return s == item || s.starts_with(item + "/");
                        });
                    }
                    return editorSelection_.isAssetSelected(item);
                },
                [this](const std::string& item) { editorSelection_.setInspectedAsset(item); }) {
        searchBuffer_[0] = '\0';
    }

    void draw() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float assetsHeight = ceil(viewport->Size.y * ASSETS_HEIGHT_RATIO);
        float startY = viewport->Size.y - assetsHeight;
        float width = viewport->Size.x * (1.0f - INSPECTOR_WIDTH_RATIO);

        ImVec2 position = {viewport->Pos.x, viewport->Pos.y + startY};
        ImVec2 size = {width, assetsHeight};
        EditorPanel::draw("Assets", position, size);
    }

    void drawBody() override {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            assetQuickActions_.deleteSelection();
        }
        drawSearchBar();
        drawAssetTable();
    }

private:
    AssetStore& assetStore_;
    EditorSelection& editorSelection_;
    AssetQuickActions& assetQuickActions_;
    AssetThumbnailGenerator& assetThumbnailGenerator_;
    Imgui_AssetDropdownMenu dropdownMenu_;
    Imgui_FileExplorerComponent fileExplorer_;
    std::string selectedFolder_;
    std::optional<std::string> contextTargetAsset_;
    char searchBuffer_[SEARCH_BUFFER_SIZE];

    bool filterMesh_ = false;
    bool filterMat_ = false;
    bool filterShad_ = false;
    bool filterTex_ = false;
    bool filterModel_ = false;

    static constexpr std::string_view knownExtensions[] = {
            ".mesh",
            ".mat",
            ".shad",
            ".png",
            ".jpg",
            ".model",
    };

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

    bool matchesTypeFilter(const std::string& asset) const {
        bool anyActive = filterMesh_ || filterMat_ || filterShad_ || filterTex_ || filterModel_;
        if (!anyActive) return true;
        auto ext = std::filesystem::path(asset).extension().string();
        if (filterMesh_ && ext == ".mesh") return true;
        if (filterMat_ && ext == ".mat") return true;
        if (filterShad_ && ext == ".shad") return true;
        if (filterTex_ && (ext == ".png" || ext == ".jpg")) return true;
        if (filterModel_ && ext == ".model") return true;
        return false;
    }

    void drawSearchBar() {
        constexpr float height = 24.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(45, 45, 45, 220));
        ImGui::BeginChild("##SearchBar",
                          ImVec2(0, height),
                          ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        float frameH = ImGui::GetFrameHeight();
        float buttonSize = frameH;

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.25f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(38, 38, 38, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(45, 45, 45, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(52, 52, 52, 255));
        ImGui::InputTextWithHint("##AssetsSearch", "Search...", searchBuffer_, SEARCH_BUFFER_SIZE);
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 15));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 25));
        if (ImGui::Button("F", ImVec2(buttonSize, buttonSize))) {
            ImGui::OpenPopup("AssetFilterPopup");
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImguiStyling::withPopup("AssetFilterPopup", [&] {
            bool all = filterMesh_ && filterMat_ && filterShad_ && filterTex_ && filterModel_;
            if (ImGui::Selectable(all ? "Deselect All" : "Select All")) {
                filterMesh_ = filterMat_ = filterShad_ = filterTex_ = filterModel_ = !all;
            }
            ImGui::Separator();
            auto toggleItem = [&](const char* name, bool& value) {
                ImGui::Selectable(name, value);
                if (ImGui::IsItemClicked()) value = !value;
            };
            toggleItem("Meshes", filterMesh_);
            toggleItem("Materials", filterMat_);
            toggleItem("Shaders", filterShad_);
            toggleItem("Textures", filterTex_);
            toggleItem("Models", filterModel_);
        });

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::Separator();
    }

    void drawAssetTable() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));

        if (ImGui::BeginTable("AssetExplorerTable",
                              2,
                              ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
                                      ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX)) {

            ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            drawFolderTree();

            ImGui::TableSetColumnIndex(1);
            drawAssetGrid();

            ImGui::EndTable();
        }

        ImGui::PopStyleVar(2);
    }

    void drawFolderTree() {
        std::vector<std::string> items;
        for (auto& f: assetStore_.listAll()) {
            if (isKnownAsset(f)) items.push_back(f);
        }
        std::ranges::sort(items);
        fileExplorer_.setItems(std::move(items));

        ImGui::BeginChild("##FolderTree", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        bool rightClicked = fileExplorer_.draw();
        if (rightClicked) {
            ImGui::OpenPopup("FolderTreeContextMenu");
        }

        dropdownMenu_.draw(
                contextTargetAsset_, selectedFolder_, "FolderTreeContextMenu", editorSelection_.getSelectedAssetIds());

        ImGui::EndChild();
    }

    void drawAssetGrid() {
        ImGui::BeginChild("##AssetGrid", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);

        auto allFiles = assetStore_.listAll();
        std::vector<std::string> assets;
        for (auto& file: allFiles) {
            if (!isKnownAsset(file)) continue;
            std::filesystem::path p(file);
            std::string folder = p.has_parent_path() ? p.parent_path().string() : "";
            if (!selectedFolder_.empty() && folder != selectedFolder_ && !folder.starts_with(selectedFolder_ + "/"))
                continue;
            if (!matchesSearch(file)) continue;
            if (!matchesTypeFilter(file)) continue;
            assets.push_back(std::move(file));
        }
        std::ranges::sort(assets);

        constexpr float thumbSize = 96.0f;
        constexpr float padding = 8.0f;
        constexpr float cellSize = thumbSize + padding * 2.0f;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, static_cast<int>(panelWidth / cellSize));

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

            std::string filename = std::filesystem::path(asset).filename().string();
            constexpr std::string_view ellipsis = "...";
            float ellipsisWidth = ImGui::CalcTextSize(ellipsis.data()).x;
            float textWidth = ImGui::CalcTextSize(filename.c_str()).x;
            std::string displayName = filename;
            if (textWidth > cellSize) {
                displayName = filename;
                while (!displayName.empty() && ImGui::CalcTextSize(displayName.c_str()).x + ellipsisWidth > cellSize) {
                    displayName.pop_back();
                }
                displayName += ellipsis;
                textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
            }
            float textOffsetX = std::max(0.0f, (cellSize - textWidth) / 2.0f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffsetX);
            ImGui::TextUnformatted(displayName.c_str());

            ImGui::EndGroup();

            bool hovered = ImGui::IsItemHovered();
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (cmdDown && selected) {
                    editorSelection_.removeFromSelection(asset);
                } else {
                    editorSelection_.addToSelection(asset, cmdDown);
                }
            }
            if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                editorSelection_.setInspectedAsset(asset);
            }

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                contextTargetAsset_.emplace(asset);
                itemRightClicked = true;
            }

            col = (col + 1) % columns;
            if (col == 0) {
                rowY = ImGui::GetCursorPosY();
            }
        }

        bool windowHovered =
                ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        if (windowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (!itemRightClicked) {
                contextTargetAsset_.reset();
            }
            ImGui::OpenPopup("AssetGridContextMenu");
        }

        dropdownMenu_.draw(
                contextTargetAsset_, selectedFolder_, "AssetGridContextMenu", editorSelection_.getSelectedAssetIds());

        ImGui::EndChild();
    }
};
