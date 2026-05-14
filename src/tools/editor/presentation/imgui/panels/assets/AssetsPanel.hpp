#pragma once
#include <array>
#include <filesystem>
#include <imgui.h>
#include <string>
#include "../../../../business/ObjectSelection.hpp"
#include "../../ImguiStyling.hpp"
#include "../EditorPanel.hpp"
#include "AssetsDropdownMenu.hpp"

class MaterialMutations;
class EditorAssetRepository;

class AssetsPanel : public EditorPanel {
public:
    static constexpr float ASSETS_HEIGHT_RATIO = 0.3f;
    static constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;
    static constexpr int SEARCH_BUFFER_SIZE = 256;

    AssetsPanel(EditorAssetRepository& repository,
                ObjectSelection& objectSelection,
                MaterialMutations& materialMutations) :
        repository_(repository),
        objectSelection_(objectSelection),
        dropdownMenu_(objectSelection, materialMutations) {
        searchBuffer_[0] = '\0';
    }

    void draw() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        float assetsHeight = ceil(viewport->Size.y * ASSETS_HEIGHT_RATIO);
        float startY = viewport->Size.y - assetsHeight;
        float width = viewport->Size.x * (1.0f - INSPECTOR_WIDTH_RATIO);

        ImVec2 position = {viewport->Pos.x, viewport->Pos.y + startY};
        ImVec2 size = {width, assetsHeight};
        EditorPanel::draw("Assets", position, size);
    }

    void drawBody() override {
        drawSearchBar();
        drawAssetTable();
    }

private:
    EditorAssetRepository& repository_;
    ObjectSelection& objectSelection_;
    AssetsDropdownMenu dropdownMenu_;
    char searchBuffer_[SEARCH_BUFFER_SIZE];

    bool filterMesh_ = false;
    bool filterMat_ = false;
    bool filterShad_ = false;
    bool filterTex_ = false;
    bool filterModel_ = false;
    std::optional<std::string> contextTargetAsset_;

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

        // -------- SEARCH --------
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.25f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(38, 38, 38, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(45, 45, 45, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(52, 52, 52, 255));
        ImGui::InputTextWithHint("##AssetsSearch", "Search...", searchBuffer_, SEARCH_BUFFER_SIZE);
        ImGui::PopStyleColor(3);

        // -------- FILTER --------
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

        // -------- POPUP --------
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 2.0f));
        ImGui::BeginChild("##AssetsTable",
                          ImVec2(0, 0),
                          ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));

        if (ImGui::BeginTable("AssetsTable",
                              2,
                              ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
                                      ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX)) {

            ImGui::TableSetupColumn(
                    "Assets", ImGuiTableColumnFlags_WidthFixed, ImGui::GetContentRegionAvail().x * 0.25f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            drawAssetsList();

            ImGui::TableSetColumnIndex(1);

            ImGui::EndTable();
        }

        ImGui::PopStyleVar(2);
        ImGui::EndChild();
        ImGui::PopStyleVar(1);
    }

    bool matchesSearch(const std::string& asset) const {
        if (searchBuffer_[0] == '\0') return true;

        std::string s = searchBuffer_;
        std::string a = asset;

        std::ranges::transform(s, s.begin(), tolower);
        std::ranges::transform(a, a.begin(), tolower);

        return a.find(s) != std::string::npos;
    }

    bool matchesFilter(const std::string& asset) const {
        bool any = filterMesh_ || filterMat_ || filterShad_ || filterTex_ || filterModel_;
        if (!any) return true;

        if (asset.ends_with(".mesh")) return filterMesh_;
        if (asset.ends_with(".mat")) return filterMat_;
        if (asset.ends_with(".shad")) return filterShad_;
        if (asset.ends_with(".png") || asset.ends_with(".jpg")) return filterTex_;
        if (asset.ends_with(".model")) return filterModel_;

        return false;
    }

    void drawAssetsList() {
        static const std::array knownExtensions = {
                std::string_view(".mesh"),
                std::string_view(".mat"),
                std::string_view(".shad"),
                std::string_view(".png"),
                std::string_view(".jpg"),
                std::string_view(".model"),
        };

        auto allFiles = repository_.listAll();
        std::vector<std::string> allAssets;
        allAssets.reserve(allFiles.size());
        for (auto& file: allFiles) {
            auto ext = std::filesystem::path(file).extension().string();
            for (auto& known: knownExtensions) {
                if (ext == known) {
                    allAssets.push_back(std::move(file));
                    break;
                }
            }
        }

        std::ranges::sort(allAssets);

        auto selectedAsset = objectSelection_.getSelectedAssetId();
        for (const auto& asset: allAssets) {
            if (matchesSearch(asset) && matchesFilter(asset)) {
                bool selected = selectedAsset && *selectedAsset == asset;
                if (ImGui::Selectable(asset.c_str(), selected)) {
                    objectSelection_.selectAsset(asset);
                }

                if (asset.ends_with(".mat") && ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("MATERIAL_ASSET", asset.c_str(), asset.size() + 1);
                    ImGui::TextUnformatted(asset.c_str());
                    ImGui::EndDragDropSource();
                }

                if ((asset.ends_with(".png") || asset.ends_with(".jpg")) && ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("texture_asset", asset.c_str(), asset.size() + 1);
                    ImGui::TextUnformatted(asset.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    contextTargetAsset_.emplace(asset);
                    ImGui::OpenPopup("AssetsContextMenu");
                }
            }
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered()) {
            contextTargetAsset_.reset();
            ImGui::OpenPopup("AssetsContextMenu");
        }

        ImguiStyling::withPopup("AssetsContextMenu", [&] { dropdownMenu_.draw(contextTargetAsset_); });
    }
};
