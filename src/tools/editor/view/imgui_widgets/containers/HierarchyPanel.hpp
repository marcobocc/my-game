#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../controller/SceneMutationsController.hpp"
#include "GameEngine.hpp"
#include "systems/assets/BuiltinAssetNames.hpp"
#include "systems/ui/ImguiWidget.hpp"

class HierarchyPanel : public ImguiWidget {
public:
    static constexpr float PANEL_WIDTH_RATIO = 0.15f;

    HierarchyPanel(std::optional<std::string>* selectedObjectId,
                   GameEngine& engine,
                   SceneMutationsController& mutations) :
        selectedObjectId_(selectedObjectId),
        engine_(engine),
        mutations_(mutations) {}

    void draw() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        ImGui::SetNextWindowPos({viewport->Pos.x, viewport->Pos.y + menuBarHeight});
        ImGui::SetNextWindowSize({width, viewport->Size.y - menuBarHeight});
        ImGui::SetNextWindowBgAlpha(0.95f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Hierarchy", nullptr, flags);

        ImGui::Spacing();
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Objects Hierarchy");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (const auto& [id, obj]: engine_.getObjects()) {
            bool isSelected = selectedObjectId_->has_value() && **selectedObjectId_ == id;
            if (ImGui::Selectable(id.c_str(), isSelected))
                *selectedObjectId_ = (isSelected) ? std::nullopt : std::optional<std::string>(id);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        drawAddObject();

        ImGui::End();
    }

private:
    std::optional<std::string>* selectedObjectId_;
    GameEngine& engine_;
    SceneMutationsController& mutations_;
    uint32_t sphereResolution_ = 5;
    bool creatingSpherePrimitive_ = false;

    void drawAddObject() {
        if (!ImGui::CollapsingHeader("Add Object")) return;

        ImGui::Indent();
        if (ImGui::CollapsingHeader("Primitives")) {
            if (ImGui::Selectable("  Cube")) {
                auto id = engine_.createMesh(PRIMITIVE_GEOMETRY_CUBE, {});
                *selectedObjectId_ = id;
            }
            if (ImGui::Selectable("  Plane")) {
                auto id = engine_.createMesh(PRIMITIVE_GEOMETRY_PLANE, {});
                *selectedObjectId_ = id;
            }

            if (ImGui::Selectable("  Sphere")) {
                creatingSpherePrimitive_ = !creatingSpherePrimitive_;
            }

            if (creatingSpherePrimitive_) {
                ImGui::SliderInt("    Resolution##sphereRes", reinterpret_cast<int*>(&sphereResolution_), 4, 5);
                if (ImGui::Button("    Create##sphere", ImVec2(-1, 0))) {
                    std::string meshName = "_PRIMITIVE_SPHERE_" + std::to_string(sphereResolution_);
                    auto id = engine_.createMesh(meshName, {});
                    *selectedObjectId_ = id;
                    creatingSpherePrimitive_ = false;
                }
            }
        }

        if (ImGui::CollapsingHeader("Models")) {
            auto models = engine_.getAvailableAssets(".model");
            if (models.empty()) {
                ImGui::TextDisabled("  No models found");
            } else {
                for (const auto& modelName: models) {
                    if (ImGui::Selectable(("  " + modelName).c_str())) {
                        auto id = mutations_.createModel(modelName, {});
                        *selectedObjectId_ = id;
                    }
                }
            }
        }
        ImGui::Unindent();
    }
};
