#pragma once
#include <imgui.h>
#include <string>
#include "../../../services/ActionDispatcher.hpp"
#include "../../../services/ClipboardService.hpp"
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetStore.hpp"

#include "../../../services/common_editing/SceneQuickActions.hpp"
#include "../../../styling/DropdownMenuComponent.hpp"
#include "../../input_handling/ShortcutBindingService.hpp"
#include "Imgui_SpherePopupModal.hpp"

class Imgui_HierarchyDropdownMenu : public DropdownMenuComponent {
public:
    Imgui_HierarchyDropdownMenu(AssetStore& assetStore,
                                SceneQuickActions& sceneQuickActions,
                                ActionDispatcher& actionDispatcher,
                                ShortcutBindingService& shortcutBindingService,
                                EditorSelection& editorSelection,
                                ClipboardService& clipboardService,
                                Imgui_SpherePopupModal* spherePopupModal = nullptr) :
        assetStore_(assetStore),
        sceneQuickActions_(sceneQuickActions),
        actionDispatcher_(actionDispatcher),
        shortcutBindingService_(shortcutBindingService),
        editorSelection_(editorSelection),
        clipboardService_(clipboardService),
        spherePopupModal_(spherePopupModal) {}

    void draw(const char* popupId) {
        bool hasSelection = !editorSelection_.getSelectedEntityIds().empty();
        bool hasClipboard = clipboardService_.hasEntity();
        constexpr ActionID editActions[] = {
                ActionID::COPY, ActionID::CUT, ActionID::DUPLICATE, ActionID::PASTE, ActionID::DELETE};
        constexpr const char* actionNames[] = {"Copy", "Cut", "Duplicate", "Paste", "Delete"};
        const bool enabledStates[] = {hasSelection, hasSelection, hasSelection, hasClipboard, hasSelection};

        std::vector<MenuItem> items;
        for (int i = 0; i < 5; ++i) {
            ActionID actionId = editActions[i];
            items.push_back({.label = actionNames[i],
                             .shortcut = shortcutBindingService_.getShortcut(actionId),
                             .enabled = enabledStates[i],
                             .onItemClick = [this, actionId] { actionDispatcher_.execute(actionId); }});
        }
        setMenuItems(std::move(items));
        DropdownMenuComponent::draw(popupId);
    }

protected:
    void drawBody() override {
        if (ImGui::MenuItem("Empty Object")) {
            sceneQuickActions_.createEmptyObject();
        }
        if (ImGui::MenuItem("Light")) {
            sceneQuickActions_.createLight();
        }
        if (ImGui::BeginMenu("3D Object")) {
            if (ImGui::MenuItem("Cube")) {
                sceneQuickActions_.createCube();
            }
            if (ImGui::MenuItem("Plane")) {
                sceneQuickActions_.createPlane();
            }
            if (ImGui::MenuItem("Sphere")) {
                if (spherePopupModal_) spherePopupModal_->requestCreation();
            }
            ImGui::Separator();
            drawModelsSubmenu();
            ImGui::EndMenu();
        }
    }

private:
    AssetStore& assetStore_;
    SceneQuickActions& sceneQuickActions_;
    ActionDispatcher& actionDispatcher_;
    ShortcutBindingService& shortcutBindingService_;
    EditorSelection& editorSelection_;
    ClipboardService& clipboardService_;
    Imgui_SpherePopupModal* spherePopupModal_;

    void drawModelsSubmenu() {
        if (ImGui::BeginMenu("Models")) {
            auto models = assetStore_.list(".model");
            if (models.empty()) {
                ImGui::MenuItem("No models found", nullptr, false, false);
            } else {
                for (const auto& modelName: models) {
                    if (ImGui::MenuItem(modelName.c_str())) {
                        sceneQuickActions_.addModel(modelName);
                    }
                }
            }
            ImGui::EndMenu();
        }
    }
};
