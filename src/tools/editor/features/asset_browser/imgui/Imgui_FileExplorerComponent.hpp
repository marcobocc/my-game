#pragma once
#include <functional>
#include <imgui.h>
#include <optional>
#include <set>
#include <string>
#include <vector>

class Imgui_FileExplorerComponent {
public:
    using SelectCallback = std::function<void(const std::string&, bool isFolder, bool cmdDown)>;
    using RightClickCallback = std::function<void(std::optional<std::string> item, bool isFolder)>;
    using IsSelectedCallback = std::function<bool(const std::string&, bool isFolder)>;
    using DoubleClickCallback = std::function<void(const std::string&)>;

    Imgui_FileExplorerComponent(std::vector<std::string> items,
                                SelectCallback onItemSelected,
                                RightClickCallback onRightClick,
                                IsSelectedCallback isSelected,
                                DoubleClickCallback onDoubleClick = {}) :
        items_(std::move(items)),
        onItemSelected_(std::move(onItemSelected)),
        onRightClick_(std::move(onRightClick)),
        isSelected_(std::move(isSelected)),
        onDoubleClick_(std::move(onDoubleClick)) {}

    void setItems(std::vector<std::string> items) { items_ = std::move(items); }

    // Returns true if a right-click occurred this frame (on an item or background).
    // Caller is responsible for opening the popup after draw() returns.
    bool draw() {
        auto [folders, leaves] = buildTree();

        bool itemRightClicked = false;
        drawLevel(folders, leaves, "", itemRightClicked);

        bool windowHovered =
                ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        if (windowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !itemRightClicked) {
            onRightClick_(std::nullopt, false);
            return true;
        }
        return itemRightClicked;
    }

private:
    std::vector<std::string> items_;
    SelectCallback onItemSelected_;
    RightClickCallback onRightClick_;
    IsSelectedCallback isSelected_;
    DoubleClickCallback onDoubleClick_;

    struct TreeData {
        std::set<std::string> folders;
        std::vector<std::string> leaves;
    };

    TreeData buildTree() const {
        TreeData data;
        for (const auto& item: items_) {
            auto slash = item.rfind('/');
            if (slash != std::string::npos) {
                std::string folder = item.substr(0, slash);
                std::string ancestor;
                for (char c: folder) {
                    if (c == '/') data.folders.insert(ancestor);
                    ancestor += c;
                }
                data.folders.insert(folder);
            }
            data.leaves.push_back(item);
        }
        return data;
    }

    void drawLevel(const std::set<std::string>& folders,
                   const std::vector<std::string>& leaves,
                   const std::string& parent,
                   bool& itemRightClicked) {
        std::set<std::string> rendered;
        for (const auto& folder: folders) {
            if (folder.empty()) continue;
            if (parent.empty()) {
                if (folder.find('/') != std::string::npos) continue;
            } else {
                if (!folder.starts_with(parent + "/")) continue;
                if (folder.substr(parent.size() + 1).find('/') != std::string::npos) continue;
            }
            if (rendered.count(folder)) continue;
            rendered.insert(folder);

            std::string label = parent.empty() ? folder : folder.substr(parent.size() + 1);
            bool folderSelected = isSelected_(folder, true);

            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            bool open = ImGui::TreeNode(label.c_str());
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
                bool cmdDown = ImGui::GetIO().KeySuper || ImGui::GetIO().KeyCtrl;
                onItemSelected_(folder, true, cmdDown);
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                itemRightClicked = true;
                onRightClick_(folder, true);
            }
            if (open) {
                drawLevel(folders, leaves, folder, itemRightClicked);
                ImGui::TreePop();
            }
        }

        for (const auto& item: leaves) {
            auto slash = item.rfind('/');
            std::string itemFolder = (slash != std::string::npos) ? item.substr(0, slash) : "";
            if (itemFolder != parent) continue;

            std::string label = (slash != std::string::npos) ? item.substr(slash + 1) : item;
            bool selected = isSelected_(item, false);
            ImGui::Selectable(label.c_str(), selected);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                bool cmdDown = ImGui::GetIO().KeySuper || ImGui::GetIO().KeyCtrl;
                onItemSelected_(item, false, cmdDown);
            }
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
                if (onDoubleClick_) onDoubleClick_(item);
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                itemRightClicked = true;
                onRightClick_(item, false);
            }
        }
    }
};
