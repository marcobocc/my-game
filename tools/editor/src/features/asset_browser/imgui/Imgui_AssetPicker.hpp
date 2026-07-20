#pragma once
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>
#include "../../../styling/SearchBar.hpp"
#include "Imgui_AssetGridView.hpp"

// Floating asset picker window. Opened by inspector fields with an extension
// filter; double-clicking an asset confirms the pick and closes the window.
class Imgui_AssetPicker {
public:
    static constexpr int SEARCH_BUFFER_SIZE = 256;

    Imgui_AssetPicker(AssetStore& assetStore, AssetThumbnailGenerator& assetThumbnailGenerator) :
        gridView_(assetStore, assetThumbnailGenerator) {
        searchBuffer_[0] = '\0';
    }

    void open(std::string title, std::vector<std::string> extensions, std::function<void(const std::string&)> onPick) {
        title_ = std::move(title);
        extensions_ = std::move(extensions);
        onPick_ = std::move(onPick);
        selected_.clear();
        searchBuffer_[0] = '\0';
        open_ = true;
        justOpened_ = true;
    }

    void draw() {
        if (!open_) return;

        if (justOpened_) {
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, {0.5f, 0.5f});
            ImGui::SetNextWindowSize({480.0f, 360.0f}, ImGuiCond_Always);
            ImGui::SetNextWindowFocus();
            justOpened_ = false;
        }

        std::string windowName = title_ + "###AssetPicker";
        if (ImGui::Begin(windowName.c_str(), &open_)) {
            drawSearchBar("AssetPickerSearch", searchBuffer_, SEARCH_BUFFER_SIZE);

            ImGui::BeginChild("##AssetPickerScroll", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);
            Imgui_AssetGridView::Callbacks cb;
            cb.isSelected = [this](const std::string& asset) { return asset == selected_; };
            cb.onClick = [this](const std::string& asset) { selected_ = asset; };
            cb.onDoubleClick = [this](const std::string& asset) {
                if (onPick_) onPick_(asset);
                open_ = false;
            };
            gridView_.draw(gridView_.listAssets(extensions_, searchBuffer_), cb);
            ImGui::EndChild();
        }
        ImGui::End();
    }

private:
    Imgui_AssetGridView gridView_;
    std::string title_ = "Select Asset";
    std::vector<std::string> extensions_;
    std::function<void(const std::string&)> onPick_;
    std::string selected_;
    char searchBuffer_[SEARCH_BUFFER_SIZE];
    bool open_ = false;
    bool justOpened_ = false;
};
