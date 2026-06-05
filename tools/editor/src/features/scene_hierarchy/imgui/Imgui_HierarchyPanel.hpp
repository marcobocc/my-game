#pragma once
#include <GLFW/glfw3.h>
#include <array>
#include <cstring>
#include <imgui.h>
#include <string>
#include "../../../services/ActionDispatcher.hpp"
#include "../../../services/ClipboardService.hpp"
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetStore.hpp"
#include "../../../services/common_editing/RuntimeScene.hpp"
#include "../../../services/common_editing/SceneQuickActions.hpp"
#include "../../../styling/EditorPanel.hpp"
#include "../../../styling/ImguiStyling.hpp"
#include "../../input_handling/ShortcutBindingService.hpp"
#include "Imgui_HierarchyDropdownMenu.hpp"
#include "Imgui_SpherePopupModal.hpp"
#include "modules/scene/components/Metadata.hpp"

class Imgui_HierarchyPanel : public EditorPanel {
public:
    static constexpr float PANEL_WIDTH_RATIO = 0.15f;
    static constexpr float ASSETS_HEIGHT_RATIO = 0.3f;

    Imgui_HierarchyPanel(AssetStore& assetStore,
                         EditorSelection& editorSelection,
                         RuntimeScene& scene,
                         ActionDispatcher& actionDispatcher,
                         ShortcutBindingService& shortcutBindingService,
                         SceneQuickActions& sceneQuickActions,
                         ClipboardService& clipboardService) :
        assetStore_(assetStore),
        editorSelection_(editorSelection),
        scene_(scene),
        sceneQuickActions_(sceneQuickActions),
        spherePopupModal_(sceneQuickActions),
        dropdownMenu_(assetStore,
                      scene,
                      sceneQuickActions,
                      actionDispatcher,
                      shortcutBindingService,
                      editorSelection,
                      clipboardService,
                      this,
                      &spherePopupModal_) {}

    void draw() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        float assetsHeight = viewport->Size.y * ASSETS_HEIGHT_RATIO;
        float height = viewport->Size.y - menuBarHeight - assetsHeight;

        ImVec2 position = {viewport->Pos.x, viewport->Pos.y + menuBarHeight};
        ImVec2 size = {width, height};
        EditorPanel::draw("Objects Hierarchy", position, size);
    }

    void startRenaming(EntityHandle entity, const std::string& currentName) {
        editingEntity_ = entity;
        editBuffer_.fill('\0');
        std::strncpy(editBuffer_.data(), currentName.c_str(), editBuffer_.size() - 1);
    }

    void drawBody() override {
        contextTargetId.reset();
        spherePopupModal_.draw();
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            // routed through ActionDispatcher -> SceneQuickActions
        }
        ImGui::Separator();

        bool cmdDown = ImGui::GetIO().KeySuper || ImGui::GetIO().KeyCtrl;
        bool rightClickedThisFrame = false;
        std::optional<EntityHandle> rightClickedEntity;

        SceneDTO dto = scene_.snapshotScene();
        for (const GameObjectDTO& obj: dto.objects) {
            EntityHandle e = obj.handle;
            std::string label;
            for (const auto& c: obj.components) {
                if (const auto* meta = std::get_if<Metadata>(&c)) {
                    label = meta->displayName.empty() ? "Entity " + std::to_string(e) : meta->displayName;
                    break;
                }
            }
            if (label.empty()) label = "Entity " + std::to_string(e);

            bool selected = editorSelection_.isEntitySelected(e);
            bool isContextTarget = contextTargetId && *contextTargetId == e;
            ImGui::Selectable(
                    ("##entity" + std::to_string(e)).c_str(), selected, ImGuiSelectableFlags_AllowOverlap, {0, 0});
            bool hovered = ImGui::IsItemHovered() || isContextTarget;
            ImGui::SameLine();

            if (editingEntity_ == e) {
                if (!editingStartedThisFrame_) {
                    ImGui::SetKeyboardFocusHere();
                    editingStartedThisFrame_ = true;
                }

                if (ImGui::InputText(("##rename_" + std::to_string(e)).c_str(),
                                     editBuffer_.data(),
                                     editBuffer_.size(),
                                     ImGuiInputTextFlags_EnterReturnsTrue)) {
                    std::string newName(editBuffer_.data());
                    if (!newName.empty()) {
                        sceneQuickActions_.renameObject(e, newName);
                    }
                    editingEntity_.reset();
                    editBuffer_.fill('\0');
                    editingStartedThisFrame_ = false;
                }

                bool itemActive = ImGui::IsItemActive();
                if (!itemActive && editingStartedThisFrame_) {
                    // Still on the same frame, wait for focus
                } else if (!itemActive) {
                    // Lost focus, cancel edit
                    editingEntity_.reset();
                    editBuffer_.fill('\0');
                    editingStartedThisFrame_ = false;
                }

                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    editingEntity_.reset();
                    editBuffer_.fill('\0');
                    editingStartedThisFrame_ = false;
                }
            } else {
                ImGui::TextUnformatted(label.c_str());
            }

            hovered = hovered || ImGui::IsItemHovered();

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (cmdDown && editorSelection_.isEntitySelected(e)) {
                    editorSelection_.removeFromSelection(e);
                } else {
                    editorSelection_.addToSelection(e, cmdDown);
                }
            }
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                rightClickedThisFrame = true;
                rightClickedEntity.emplace(e);
            }
        }

        if (!rightClickedThisFrame && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            rightClickedThisFrame = true;
            rightClickedEntity.reset();
        }

        if (rightClickedThisFrame) {
            contextTargetId = rightClickedEntity;
            if (rightClickedEntity) {
                std::string label = "Entity " + std::to_string(*rightClickedEntity);
                for (const auto& obj: dto.objects) {
                    if (obj.handle == *rightClickedEntity) {
                        for (const auto& c: obj.components) {
                            if (const auto* meta = std::get_if<Metadata>(&c)) {
                                if (!meta->displayName.empty()) label = meta->displayName;
                                break;
                            }
                        }
                        break;
                    }
                }
                dropdownMenu_.setContextEntity(*rightClickedEntity, label);
            }
            ImGui::OpenPopup("HierarchyContextMenu");
        }

        dropdownMenu_.draw("HierarchyContextMenu");

        if (auto renameEntity = dropdownMenu_.getAndClearRenameRequest()) {
            startRenaming(*renameEntity, dropdownMenu_.getRenameRequestName());
        }
    }

private:
    static constexpr size_t NAME_BUFFER_SIZE = 256;

    AssetStore& assetStore_;
    EditorSelection& editorSelection_;
    RuntimeScene& scene_;
    SceneQuickActions& sceneQuickActions_;
    Imgui_SpherePopupModal spherePopupModal_;
    Imgui_HierarchyDropdownMenu dropdownMenu_;

    std::optional<EntityHandle> contextTargetId;
    std::optional<EntityHandle> editingEntity_;
    std::array<char, NAME_BUFFER_SIZE> editBuffer_{};
    bool editingStartedThisFrame_ = false;
};
