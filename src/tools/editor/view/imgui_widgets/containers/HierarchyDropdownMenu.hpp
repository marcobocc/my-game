#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../controller/SceneMutationsController.hpp"
#include "../../../state/EditorState.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/assets/BuiltinAssetNames.hpp"
#include "systems/scene/Scene.hpp"

class HierarchyDropdownMenu {
public:
    HierarchyDropdownMenu(EditorState& editorState,
                          Scene& scene,
                          AssetManager& assetManager,
                          SceneMutationsController& mutations) :
        editorState_(editorState),
        scene_(scene),
        assetManager_(assetManager),
        mutations_(mutations) {}

    void draw() {
        if (ImGui::BeginPopupContextWindow("HierarchyContextMenu")) {
            drawMenu();
            ImGui::EndPopup();
        }
    }

private:
    EditorState& editorState_;
    Scene& scene_;
    AssetManager& assetManager_;
    SceneMutationsController& mutations_;

    void drawMenu() {
        if (ImGui::MenuItem("Empty Object")) {
            auto id = mutations_.createEmptyObject();
            editorState_.setSelectedObject(id);
        }
        if (ImGui::BeginMenu("3D Object")) {
            drawPrimitivesSubmenu();
            drawModelsSubmenu();
            ImGui::EndMenu();
        }
    }

    void drawPrimitivesSubmenu() {
        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Cube")) {
                auto id = scene_.createMesh(PRIMITIVE_GEOMETRY_CUBE, {});
                editorState_.setSelectedObject(id);
            }
            if (ImGui::MenuItem("Plane")) {
                auto id = scene_.createMesh(PRIMITIVE_GEOMETRY_PLANE, {});
                editorState_.setSelectedObject(id);
            }
            if (ImGui::MenuItem("Sphere")) {
                auto id = scene_.createMesh("_PRIMITIVE_SPHERE_5", {});
                editorState_.setSelectedObject(id);
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
                        auto id = mutations_.createModel(modelName, {});
                        editorState_.setSelectedObject(id);
                    }
                }
            }
            ImGui::EndMenu();
        }
    }
};
