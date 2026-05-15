#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../../business/ActionDispatcher.hpp"
#include "../../../../business/EditorSelection.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/scene_editing/ObjectBuilder.hpp"
#include "../../../../business/scene_editing/SceneMutations.hpp"
#include "../../../../input/ShortcutBindingService.hpp"
#include "../../../components/DropdownMenuComponent.hpp"
#include "SpherePopupModal.hpp"
#include "modules/asset_management/asset_types/Model.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Renderer.hpp"
#include "modules/scene/components/Transform.hpp"

class SceneMutations;

class HierarchyDropdownMenu : public DropdownMenuComponent {
public:
    HierarchyDropdownMenu(EditorAssetRepository& repository,
                          SceneMutations& sceneMutations,
                          ObjectBuilder& objectBuilder,
                          ActionDispatcher& actionDispatcher,
                          ShortcutBindingService& shortcutBindingService,
                          EditorSelection& editorSelection,
                          SpherePopupModal* spherePopupModal = nullptr) :
        repository_(repository),
        sceneMutations_(sceneMutations),
        objectBuilder_(objectBuilder),
        actionDispatcher_(actionDispatcher),
        shortcutBindingService_(shortcutBindingService),
        editorSelection_(editorSelection),
        spherePopupModal_(spherePopupModal) {}

    void draw(const char* popupId) {
        bool hasSelection = !editorSelection_.getSelectedEntityIds().empty();
        constexpr ActionID editActions[] = {
                ActionID::COPY, ActionID::CUT, ActionID::DUPLICATE, ActionID::PASTE, ActionID::DELETE};
        constexpr const char* actionNames[] = {"Copy", "Cut", "Duplicate", "Paste", "Delete"};

        std::vector<MenuItem> items;
        for (int i = 0; i < 5; ++i) {
            ActionID actionId = editActions[i];
            items.push_back({.label = actionNames[i],
                             .shortcut = shortcutBindingService_.getShortcut(actionId),
                             .enabled = hasSelection,
                             .onItemClick = [this, actionId] { actionDispatcher_.execute(actionId); }});
        }
        setMenuItems(std::move(items));
        DropdownMenuComponent::draw(popupId);
    }

protected:
    void drawBody() override {
        if (ImGui::MenuItem("Empty Object")) {
            sceneMutations_.createObject({{"metadata", {{"name", "Object"}}}});
        }
        if (ImGui::MenuItem("Light")) {
            createLight();
        }
        if (ImGui::BeginMenu("3D Object")) {
            if (ImGui::MenuItem("Cube")) {
                sceneMutations_.createObject(objectBuilder_.cube());
            }
            if (ImGui::MenuItem("Plane")) {
                sceneMutations_.createObject(objectBuilder_.plane());
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
    EditorAssetRepository& repository_;
    SceneMutations& sceneMutations_;
    ObjectBuilder& objectBuilder_;
    ActionDispatcher& actionDispatcher_;
    ShortcutBindingService& shortcutBindingService_;
    EditorSelection& editorSelection_;
    SpherePopupModal* spherePopupModal_;

    void drawModelsSubmenu() {
        if (ImGui::BeginMenu("Models")) {
            auto models = repository_.list(".model");
            if (models.empty()) {
                ImGui::MenuItem("No models found", nullptr, false, false);
            } else {
                for (const auto& modelName: models) {
                    if (ImGui::MenuItem(modelName.c_str())) {
                        loadModel(modelName);
                    }
                }
            }
            ImGui::EndMenu();
        }
    }

    void loadModel(const std::string& modelName) {
        const auto* model = repository_.get<Model>(modelName);
        if (!model) return;
        Transform t{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)};
        Renderer r{model->meshName, model->materialName, std::nullopt, true};
        sceneMutations_.createObject({
                {"metadata", {{"name", modelName}}},
                {"transform", t.serialize()},
                {"renderer", r.serialize()},
        });
    }

    void createLight() {
        Transform t{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)};
        Light l{LightType::Directional, 1.0f};
        sceneMutations_.createObject({
                {"metadata", {{"name", "Light"}}},
                {"transform", t.serialize()},
                {"light", l.serialize()},
        });
    }
};
