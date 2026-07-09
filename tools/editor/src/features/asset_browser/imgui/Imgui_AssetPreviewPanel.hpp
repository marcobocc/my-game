#pragma once
#include <imgui.h>
#include <string>
#include "../../../services/AssetThumbnailGenerator.hpp"
#include "../../../services/EditorSelection.hpp"
#include "../../../services/ImguiThumbnail.hpp"
#include "../../../styling/EditorPanel.hpp"
#include "../../../styling/ImguiStyling.hpp"

class Imgui_AssetPreviewPanel : public EditorPanel {
public:
    Imgui_AssetPreviewPanel(EditorSelection& editorSelection, AssetThumbnailGenerator& thumbnailGenerator) :
        editorSelection_(editorSelection),
        thumbnailGenerator_(thumbnailGenerator) {}

    void draw() { EditorPanel::draw("Asset Preview"); }

    void drawBody() override {
        const auto& inspected = editorSelection_.getInspectedAsset();
        const auto& selected = editorSelection_.getSelectedAssetIds();

        std::string asset;
        if (inspected.has_value()) {
            asset = *inspected;
        } else if (selected.size() == 1) {
            asset = *selected.begin();
        }

        if (asset.empty()) {
            ImGui::TextDisabled("No asset selected");
            return;
        }

        ImguiStyling::withBodyStyling([&] { ImGui::TextUnformatted(asset.c_str()); });

        ImguiThumbnail thumbnail = thumbnailGenerator_.get(asset);
        if (!thumbnail) {
            ImGui::TextDisabled("No preview available");
            return;
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float aspect = static_cast<float>(thumbnail.width) / static_cast<float>(thumbnail.height);
        ImVec2 imgSize;
        if (aspect >= 1.0f) {
            imgSize = {avail.x, avail.x / aspect};
        } else {
            imgSize = {avail.y * aspect, avail.y};
        }
        imgSize.x = std::min(imgSize.x, avail.x);
        imgSize.y = std::min(imgSize.y, avail.y);

        float offsetX = (avail.x - imgSize.x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        ImGui::Image((ImTextureID) thumbnail.descriptorSet, imgSize);
    }

private:
    EditorSelection& editorSelection_;
    AssetThumbnailGenerator& thumbnailGenerator_;
};
