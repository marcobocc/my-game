#pragma once
#include <filesystem>
#include <imgui.h>
#include <string>
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
    static constexpr int SEARCH_BUFFER_SIZE = 256;

    Imgui_AssetExplorerPanel(AssetStore& assetStore,
                             EditorSelection& editorSelection,
                             AssetQuickActions& assetQuickActions) :
        assetStore_(assetStore),
        editorSelection_(editorSelection),
        assetQuickActions_(assetQuickActions),
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

    void draw() { EditorPanel::draw("Assets"); }

    const std::string& getSelectedFolder() const { return selectedFolder_; }

    void drawBody() override {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            assetQuickActions_.deleteSelection();
        }
        drawSearchBar();
        drawFileList();
    }

private:
    AssetStore& assetStore_;
    EditorSelection& editorSelection_;
    AssetQuickActions& assetQuickActions_;
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
            ".lua",
            ".animctrl",
            ".prefab",
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

    void drawFileList() {
        std::vector<std::string> items;
        for (auto& f: assetStore_.listAll()) {
            if (!AssetStore::isBuiltin(f) && isKnownAsset(f) && matchesSearch(f) && matchesTypeFilter(f))
                items.push_back(f);
        }
        std::ranges::sort(items);
        fileExplorer_.setItems(std::move(items));

        ImGui::BeginChild("##FileList", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        bool rightClicked = fileExplorer_.draw();
        if (rightClicked) ImGui::OpenPopup("AssetContextMenu");

        dropdownMenu_.draw(
                contextTargetAsset_, selectedFolder_, "AssetContextMenu", editorSelection_.getSelectedAssetIds());

        ImGui::EndChild();
    }
};
