#pragma once
#include <algorithm>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>
#include "../../../services/AssetThumbnailGenerator.hpp"
#include "../../../services/ImguiThumbnail.hpp"
#include "../../../services/common_editing/AssetStore.hpp"

// Reusable thumbnail grid of assets, shared by the Asset Grid panel and the
// asset picker window. Callers own selection state and interaction semantics.
class Imgui_AssetGridView {
public:
    struct Callbacks {
        std::function<bool(const std::string&)> isSelected;
        std::function<void(const std::string&)> onClick;
        std::function<void(const std::string&)> onDoubleClick;
        std::function<void(const std::string&)> onRightClick;
        // Returns the drag-drop payload type for an asset, or nullptr for no drag source.
        std::function<const char*(const std::string&)> dragPayloadType;
    };

    Imgui_AssetGridView(AssetStore& assetStore, AssetThumbnailGenerator& assetThumbnailGenerator) :
        assetStore_(assetStore),
        assetThumbnailGenerator_(assetThumbnailGenerator) {}

    std::vector<std::string> listAssets(const std::vector<std::string>& extensions, const char* search) const {
        std::vector<std::string> assets;
        for (auto& file: assetStore_.listAll()) {
            if (file.starts_with(".cache/")) continue;
            if (AssetStore::isBuiltin(file)) continue;
            if (!matchesExtension(file, extensions)) continue;
            if (!matchesSearch(file, search)) continue;
            assets.push_back(file);
        }
        std::ranges::sort(assets);
        return assets;
    }

    void draw(const std::vector<std::string>& assets, const Callbacks& cb) {
        constexpr float thumbSize = 64.0f;
        constexpr float padding = 2.0f;
        constexpr float cellSize = thumbSize + padding * 2.0f;

        int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cellSize));

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

            if (cb.isSelected && cb.isSelected(asset)) {
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

            if (cb.dragPayloadType) {
                if (const char* payloadType = cb.dragPayloadType(asset)) {
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        ImGui::SetDragDropPayload(payloadType, asset.c_str(), asset.size() + 1);
                        ImGui::TextUnformatted(asset.c_str());
                        ImGui::EndDragDropSource();
                    }
                }
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
            if (hovered && cb.onClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) cb.onClick(asset);
            if (hovered && cb.onDoubleClick && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                cb.onDoubleClick(asset);
            if (hovered && cb.onRightClick && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) cb.onRightClick(asset);

            col = (col + 1) % columns;
            if (col == 0) rowY = ImGui::GetCursorPosY();
        }
    }

private:
    AssetStore& assetStore_;
    AssetThumbnailGenerator& assetThumbnailGenerator_;

    static bool matchesExtension(const std::string& asset, const std::vector<std::string>& extensions) {
        if (extensions.empty()) return true;
        auto ext = std::filesystem::path(asset).extension().string();
        return std::ranges::find(extensions, ext) != extensions.end();
    }

    static bool matchesSearch(const std::string& asset, const char* search) {
        if (search == nullptr || search[0] == '\0') return true;
        std::string lower = asset;
        std::string query = search;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
        return lower.find(query) != std::string::npos;
    }
};
