#pragma once
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../../business/scene_editing/ObjectPrefabs.hpp"
#include "../../../../business/scene_editing/SceneMutations.hpp"
#include "SpherePopupModal.hpp"
#include "modules/assets/AssetManager.hpp"
#include "structs/assets/Model.hpp"
#include "structs/components/Renderer.hpp"
#include "structs/components/Transform.hpp"

class SceneMutations;

class HierarchyDropdownMenu {
public:
    HierarchyDropdownMenu(AssetManager& assetManager,
                          SceneMutations& sceneMutations,
                          SpherePopupModal* spherePopupModal = nullptr) :
        assetManager_(assetManager),
        sceneMutations_(sceneMutations),
        spherePopupModal_(spherePopupModal) {}

    void draw(std::optional<EntityHandle> selectedItem) {
        if (selectedItem) {
            if (ImGui::MenuItem("Delete")) {
                sceneMutations_.destroyObject(*selectedItem);
            }
            ImGui::Separator();
        }
        if (ImGui::MenuItem("Empty Object")) {
            sceneMutations_.createObject({{"metadata", {{"name", "Object"}}}});
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
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    SpherePopupModal* spherePopupModal_;

    void drawModelsSubmenu() {
        if (ImGui::BeginMenu("Models")) {
            auto models = assetManager_.getAvailableAssets(".model");
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
        auto* model = assetManager_.get<Model>(modelName);
        if (!model) return;
        Transform t{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)};
        Renderer r{model->getMeshName(), model->getMaterialName(), std::nullopt, true};
        sceneMutations_.createObject({
                {"metadata", {{"name", modelName}}},
                {"transform", t.serialize()},
                {"renderer", r.serialize()},
        });
    }
};
