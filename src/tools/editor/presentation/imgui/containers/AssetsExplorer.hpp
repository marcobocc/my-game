#pragma once
#include <imgui.h>
#include <string>
#include "modules/assets/AssetManager.hpp"
#include "modules/ui/ImguiWidget.hpp"

class AssetsExplorer : public ImguiWidget {
public:
    static constexpr float ASSETS_HEIGHT_RATIO = 0.3f;
    static constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;
    static constexpr int SEARCH_BUFFER_SIZE = 256;

    explicit AssetsExplorer(AssetManager& assetManager) : assetManager_(assetManager) { searchBuffer_[0] = '\0'; }

    void draw() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float assetsHeight = viewport->Size.y * ASSETS_HEIGHT_RATIO;
        float startY = viewport->Size.y - assetsHeight;
        float width = viewport->Size.x * (1.0f - INSPECTOR_WIDTH_RATIO);

        ImGui::SetNextWindowPos({viewport->Pos.x, viewport->Pos.y + startY});
        ImGui::SetNextWindowSize({width, assetsHeight});
        ImGui::SetNextWindowBgAlpha(0.95f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("AssetsExplorer", nullptr, flags);

        ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - windowPadding.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - windowPadding.y);

        float frameHeight = ImGui::GetFrameHeight();
        float barHeight = frameHeight + windowPadding.y * 0.5f;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 barMin = ImGui::GetCursorScreenPos();
        ImVec2 barMax = {barMin.x + ImGui::GetContentRegionAvail().x + windowPadding.x, barMin.y + barHeight};
        drawList->AddRectFilled(barMin, barMax, ImGui::GetColorU32({0.2f, 0.2f, 0.2f, 0.5f}));

        ImGui::SetCursorPos({windowPadding.x, ImGui::GetCursorPosY() + windowPadding.y * 0.5f});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6.0f, 3.0f});
        ImGui::SetWindowFontScale(0.85f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.15f);
        ImGui::InputTextWithHint("##AssetsSearch", "Search...", searchBuffer_, SEARCH_BUFFER_SIZE);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleVar(2);

        ImGui::SameLine(0.0f, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::SetWindowFontScale(0.85f);
        if (ImGui::Button("F", {18, ImGui::GetFrameHeight()})) {
            ImGui::OpenPopup("AssetFilterPopup");
        }
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleVar();

        if (ImGui::BeginPopup("AssetFilterPopup")) {
            bool allSelected = filterMesh_ && filterMat_ && filterShad_ && filterTex_ && filterModel_;
            ImGui::Text(allSelected ? "Deselect All" : "Select All");
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                filterMesh_ = filterMat_ = filterShad_ = filterTex_ = filterModel_ = !allSelected;
            }
            ImGui::Separator();
            if (filterMesh_) {
                ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Meshes");
            } else {
                ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "Meshes");
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) filterMesh_ = !filterMesh_;
            if (filterMat_) {
                ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Materials");
            } else {
                ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "Materials");
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) filterMat_ = !filterMat_;
            if (filterShad_) {
                ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Shaders");
            } else {
                ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "Shaders");
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) filterShad_ = !filterShad_;
            if (filterTex_) {
                ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Textures");
            } else {
                ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "Textures");
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) filterTex_ = !filterTex_;
            if (filterModel_) {
                ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Models");
            } else {
                ImGui::TextColored({0.7f, 0.7f, 0.7f, 1.0f}, "Models");
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) filterModel_ = !filterModel_;
            ImGui::EndPopup();
        }

        float windowTop = ImGui::GetWindowPos().y;
        float barHeightFromWindowTop = barMax.y - windowTop;
        ImGui::SetCursorPosY(barHeightFromWindowTop);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0, 0});
        if (ImGui::BeginTable("AssetsTable",
                              2,
                              ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn(
                    "Assets", ImGuiTableColumnFlags_WidthFixed, ImGui::GetContentRegionAvail().x * 0.25f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            drawAssetsList();

            ImGui::TableSetColumnIndex(1);
            // TODO: Right subpanel

            ImGui::EndTable();
        }
        ImGui::PopStyleVar(2);

        ImGui::End();
    }

private:
    AssetManager& assetManager_;
    char searchBuffer_[SEARCH_BUFFER_SIZE];
    bool filterMesh_ = false;
    bool filterMat_ = false;
    bool filterShad_ = false;
    bool filterTex_ = false;
    bool filterModel_ = false;

    bool matchesSearch(const std::string& asset) const {
        if (searchBuffer_[0] == '\0') return true;
        std::string search = searchBuffer_;
        std::ranges::transform(search, search.begin(), tolower);
        std::string assetLower = asset;
        std::ranges::transform(assetLower, assetLower.begin(), tolower);
        return assetLower.find(search) != std::string::npos;
    }

    bool matchesFilter(const std::string& asset) const {
        bool anyFilterActive = filterMesh_ || filterMat_ || filterShad_ || filterTex_ || filterModel_;
        if (!anyFilterActive) return true;

        if (asset.ends_with(".mesh")) return filterMesh_;
        if (asset.ends_with(".mat")) return filterMat_;
        if (asset.ends_with(".shad")) return filterShad_;
        if (asset.ends_with(".tex")) return filterTex_;
        if (asset.ends_with(".model")) return filterModel_;
        return false;
    }

    void drawAssetsList() const {
        std::vector<std::string> allAssets;
        auto meshes = assetManager_.getAvailableAssets(".mesh");
        auto materials = assetManager_.getAvailableAssets(".mat");
        auto shaders = assetManager_.getAvailableAssets(".shad");
        auto textures = assetManager_.getAvailableAssets(".tex");
        auto models = assetManager_.getAvailableAssets(".model");

        allAssets.insert(allAssets.end(), meshes.begin(), meshes.end());
        allAssets.insert(allAssets.end(), materials.begin(), materials.end());
        allAssets.insert(allAssets.end(), shaders.begin(), shaders.end());
        allAssets.insert(allAssets.end(), textures.begin(), textures.end());
        allAssets.insert(allAssets.end(), models.begin(), models.end());

        std::ranges::sort(allAssets);

        ImGui::Spacing();
        for (const auto& asset: allAssets) {
            if (matchesSearch(asset) && matchesFilter(asset)) {
                ImGui::Selectable(asset.c_str());
            }
        }
    }
};
