#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "systems/assets/AssetManager.hpp"
#include "systems/assets/BuiltinAssetNames.hpp"

class SceneMutations;

class HierarchyDropdownMenu {
public:
    HierarchyDropdownMenu(AssetManager& assetManager, SceneMutations& sceneMutations) :
        assetManager_(assetManager),
        sceneMutations_(sceneMutations) {}

    void draw(std::optional<std::string> selectedItem) {
        if (selectedItem) {
            if (ImGui::MenuItem("Delete")) {
                sceneMutations_.deleteObject(*selectedItem);
            }
            ImGui::Separator();
        }
        if (ImGui::MenuItem("Empty Object")) {
            sceneMutations_.createEmptyObjectAndSelect();
        }
        if (ImGui::BeginMenu("3D Object")) {
            drawPrimitivesSubmenu();
            drawModelsSubmenu();
            ImGui::EndMenu();
        }
    }


private:
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;

    void drawPrimitivesSubmenu() {
        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Cube")) {
                sceneMutations_.createCube({});
            }
            if (ImGui::MenuItem("Plane")) {
                sceneMutations_.createPlane({});
            }
            if (ImGui::MenuItem("Sphere")) {
                sceneMutations_.createSphere({});
            }
            ImGui::EndMenu();
        }
    }

    void drawModelsSubmenu() {
        if (ImGui::BeginMenu("Models")) {
            auto models = assetManager_.getAvailableAssets(".model");
            if (models.empty()) {
                ImGui::MenuItem("No models found", nullptr, false, false);
            } else {
                for (const auto& modelName: models) {
                    if (ImGui::MenuItem(modelName.c_str())) {
                        sceneMutations_.createModelAndSelect(modelName, {});
                    }
                }
            }
            ImGui::EndMenu();
        }
    }
};
