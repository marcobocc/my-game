#pragma once
#include <imgui.h>
#include <string>
#include "../../../services/ActionDispatcher.hpp"
#include "../../../services/ClipboardService.hpp"
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetStore.hpp"
#include "../../../services/common_editing/RuntimeScene.hpp"
#include "../../../services/common_editing/SceneQuickActions.hpp"
#include "../../../styling/DropdownMenuComponent.hpp"
#include "../../input_handling/ShortcutBindingService.hpp"
#include "Imgui_CapsulePopupModal.hpp"
#include "Imgui_SpherePopupModal.hpp"
#include "modules/scene/components/Metadata.hpp"

class Imgui_HierarchyPanel;

class Imgui_HierarchyDropdownMenu : public DropdownMenuComponent {
public:
    Imgui_HierarchyDropdownMenu(AssetStore& assetStore,
                                RuntimeScene& scene,
                                SceneQuickActions& sceneQuickActions,
                                ActionDispatcher& actionDispatcher,
                                ShortcutBindingService& shortcutBindingService,
                                EditorSelection& editorSelection,
                                ClipboardService& clipboardService,
                                Imgui_HierarchyPanel* hierarchyPanel = nullptr,
                                Imgui_SpherePopupModal* spherePopupModal = nullptr,
                                Imgui_CapsulePopupModal* capsulePopupModal = nullptr) :
        assetStore_(assetStore),
        scene_(scene),
        sceneQuickActions_(sceneQuickActions),
        actionDispatcher_(actionDispatcher),
        shortcutBindingService_(shortcutBindingService),
        editorSelection_(editorSelection),
        clipboardService_(clipboardService),
        hierarchyPanel_(hierarchyPanel),
        spherePopupModal_(spherePopupModal),
        capsulePopupModal_(capsulePopupModal) {}

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

    void setContextEntity(EntityHandle entity, std::string_view currentName) {
        contextEntity_ = entity;
        contextEntityName_ = currentName;
    }

    std::optional<EntityHandle> getAndClearRenameRequest() {
        auto result = renameRequested_;
        renameRequested_.reset();
        return result;
    }

    const std::string& getRenameRequestName() const { return contextEntityName_; }

protected:
    void drawBody() override {
        bool hasContextEntity = contextEntity_.has_value();
        bool hasSelection = !editorSelection_.getSelectedEntityIds().empty();

        if (ImGui::MenuItem("Rename", nullptr, false, hasContextEntity)) {
            if (contextEntity_) renameRequested_ = *contextEntity_;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Group Selection", nullptr, false, hasSelection)) {
            sceneQuickActions_.groupSelected("Group");
        }
        if (ImGui::MenuItem("Remove from Group", nullptr, false, hasContextEntity)) {
            if (contextEntity_) sceneQuickActions_.ungroupNode(*contextEntity_);
        }

        ImGui::Separator();

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
            if (ImGui::MenuItem("Capsule")) {
                if (capsulePopupModal_) capsulePopupModal_->requestCreation();
            }
            ImGui::Separator();
            drawModelsSubmenu();
            ImGui::EndMenu();
        }
    }

private:
    AssetStore& assetStore_;
    RuntimeScene& scene_;
    SceneQuickActions& sceneQuickActions_;
    ActionDispatcher& actionDispatcher_;
    ShortcutBindingService& shortcutBindingService_;
    EditorSelection& editorSelection_;
    ClipboardService& clipboardService_;
    Imgui_HierarchyPanel* hierarchyPanel_;
    Imgui_SpherePopupModal* spherePopupModal_;
    Imgui_CapsulePopupModal* capsulePopupModal_;
    std::optional<EntityHandle> contextEntity_;
    std::optional<EntityHandle> renameRequested_;
    std::string contextEntityName_;

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
