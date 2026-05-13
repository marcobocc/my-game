#pragma once
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../../business/ActionDispatcher.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/scene_editing/ObjectPrefabs.hpp"
#include "../../../../business/scene_editing/SceneMutations.hpp"
#include "../../../../input/ShortcutBindingService.hpp"
#include "SpherePopupModal.hpp"
#include "modules/asset_management/asset_types/Model.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Renderer.hpp"
#include "modules/scene/components/Transform.hpp"

class SceneMutations;

class HierarchyDropdownMenu {
public:
    HierarchyDropdownMenu(EditorAssetRepository& repository,
                          SceneMutations& sceneMutations,
                          ActionDispatcher& actionDispatcher,
                          ShortcutBindingService& shortcutBindingService,
                          SpherePopupModal* spherePopupModal = nullptr) :
        repository_(repository),
        sceneMutations_(sceneMutations),
        actionDispatcher_(actionDispatcher),
        shortcutBindingService_(shortcutBindingService),
        spherePopupModal_(spherePopupModal) {}

    void draw(std::optional<EntityHandle> selectedItem) {
        bool hasSelection = selectedItem.has_value();

        constexpr ActionID editActions[] = {
                ActionID::COPY, ActionID::CUT, ActionID::DUPLICATE, ActionID::PASTE, ActionID::DELETE};
        constexpr const char* actionNames[] = {"Copy", "Cut", "Duplicate", "Paste", "Delete"};

        for (int i = 0; i < 5; ++i) {
            std::string shortcutStr = shortcutBindingService_.getShortcut(editActions[i]);
            if (ImGui::MenuItem(actionNames[i], shortcutStr.c_str(), false, hasSelection)) {
                actionDispatcher_.execute(editActions[i]);
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Empty Object")) {
            sceneMutations_.createObject({{"metadata", {{"name", "Object"}}}});
        }
        if (ImGui::MenuItem("Light")) {
            createLight();
        }
        if (ImGui::BeginMenu("3D Object")) {
            if (ImGui::MenuItem("Cube")) {
                sceneMutations_.createObject(primitives::cube());
            }
            if (ImGui::MenuItem("Plane")) {
                sceneMutations_.createObject(primitives::plane());
            }
            if (ImGui::MenuItem("Sphere")) {
                if (spherePopupModal_) {
                    spherePopupModal_->requestCreation();
                }
            }
            ImGui::Separator();
            drawModelsSubmenu();
            ImGui::EndMenu();
        }
    }

private:
    EditorAssetRepository& repository_;
    SceneMutations& sceneMutations_;
    ActionDispatcher& actionDispatcher_;
    ShortcutBindingService& shortcutBindingService_;
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
        auto* model = repository_.get<Model>(modelName);
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
