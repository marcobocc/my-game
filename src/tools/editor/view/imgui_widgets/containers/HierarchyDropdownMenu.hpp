#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../controller/SceneMutationsController.hpp"
#include "../../../state/EditorState.hpp"
#include "GameEngine.hpp"
#include "systems/assets/BuiltinAssetNames.hpp"

class HierarchyDropdownMenu {
public:
    HierarchyDropdownMenu(EditorState& editorState, GameEngine& engine, SceneMutationsController& mutations) :
        editorState_(editorState),
        engine_(engine),
        mutations_(mutations) {}

    void draw() {
        if (ImGui::BeginPopupContextWindow("HierarchyContextMenu")) {
            drawMenu();
            ImGui::EndPopup();
        }
    }

private:
    EditorState& editorState_;
    GameEngine& engine_;
    SceneMutationsController& mutations_;

    void drawMenu() {
        if (ImGui::BeginMenu("3D Object")) {
            drawPrimitivesSubmenu();
            drawModelsSubmenu();
            ImGui::EndMenu();
        }
    }

    void drawPrimitivesSubmenu() {
        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Cube")) {
                auto id = engine_.createMesh(PRIMITIVE_GEOMETRY_CUBE, {});
                editorState_.setSelectedObject(id);
            }
            if (ImGui::MenuItem("Plane")) {
                auto id = engine_.createMesh(PRIMITIVE_GEOMETRY_PLANE, {});
                editorState_.setSelectedObject(id);
            }
            if (ImGui::MenuItem("Sphere")) {
                auto id = engine_.createMesh("_PRIMITIVE_SPHERE_5", {});
                editorState_.setSelectedObject(id);
            }
            ImGui::EndMenu();
        }
    }

    void drawModelsSubmenu() {
        if (ImGui::BeginMenu("Models")) {
            auto models = engine_.getAvailableAssets(".model");
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
