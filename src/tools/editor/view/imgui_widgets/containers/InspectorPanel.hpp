#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../controller/SceneMutationsController.hpp"
#include "../game_components/BoxColliderWidget.hpp"
#include "../game_components/CameraWidget.hpp"
#include "../game_components/RendererWidget.hpp"
#include "../game_components/TransformWidget.hpp"
#include "GameEngine.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "systems/ui/ImguiWidget.hpp"

class InspectorPanel : public ImguiWidget {
public:
    using AABBToggleCallback = RendererWidget::AABBToggleCallback;

    InspectorPanel(const std::optional<std::string>* selectedObjectId,
                   GameEngine& engine,
                   SceneMutationsController& mutations,
                   AABBToggleCallback aabbToggle = nullptr) :
        selectedObjectId_(selectedObjectId),
        engine_(engine),
        mutations_(mutations),
        transformWidget_(mutations),
        rendererWidget_(engine, mutations, aabbToggle),
        boxColliderWidget_(mutations),
        cameraWidget_(mutations) {}

    void draw() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        ImGui::SetNextWindowPos({viewport->Pos.x + viewport->Size.x - width, viewport->Pos.y + menuBarHeight});
        ImGui::SetNextWindowSize({width, viewport->Size.y - menuBarHeight});
        ImGui::SetNextWindowBgAlpha(0.95f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Inspector", nullptr, flags);

        ImGui::Spacing();
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Inspector");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!selectedObjectId_ || !selectedObjectId_->has_value()) {
            ImGui::TextDisabled("No object selected");
        } else {
            drawObject(engine_.getObject(**selectedObjectId_));
        }

        ImGui::End();
    }

    static constexpr float PANEL_WIDTH_RATIO = 0.25f;

private:
    const std::optional<std::string>* selectedObjectId_;
    GameEngine& engine_;
    SceneMutationsController& mutations_;
    TransformWidget transformWidget_;
    RendererWidget rendererWidget_;
    BoxColliderWidget boxColliderWidget_;
    CameraWidget cameraWidget_;

    void drawObject(GameObject& obj) const {
        if (obj.has<Transform>()) {
            transformWidget_.draw(obj.get<Transform>());
            drawRemoveButton<Transform>("##RemoveTransform", obj);
        }
        ImGui::Spacing();
        if (obj.has<Camera>()) {
            cameraWidget_.draw(obj.get<Camera>());
            drawRemoveButton<Camera>("##RemoveCamera", obj);
        }
        ImGui::Spacing();
        if (obj.has<Renderer>()) {
            rendererWidget_.draw(obj.get<Renderer>(), obj.getName());
            drawRemoveButton<Renderer>("##RemoveRenderer", obj);
        }
        ImGui::Spacing();
        if (obj.has<BoxCollider>()) {
            boxColliderWidget_.draw(obj.get<BoxCollider>());
            drawRemoveButton<BoxCollider>("##RemoveBoxCollider", obj);
        }
        ImGui::Spacing();
        drawAddComponent(obj);
    }

    template<typename T>
    void drawRemoveButton(const char* id, GameObject& obj) const {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Remove").x);
        if (ImGui::SmallButton((std::string("Remove") + id).c_str())) mutations_.removeComponent<T>(obj);
    }

    void drawAddComponent(GameObject& obj) const {
        ImGui::Separator();
        ImGui::Spacing();
        if (!ImGui::CollapsingHeader("Add Component")) return;

        ImGui::Indent();
        if (!obj.has<Transform>()) {
            if (ImGui::Selectable("  Transform")) mutations_.addComponent<Transform>(obj);
        }
        if (!obj.has<Camera>()) {
            if (ImGui::Selectable("  Camera")) mutations_.addComponent<Camera>(obj);
        }
        if (!obj.has<Renderer>()) {
            if (ImGui::Selectable("  Renderer")) mutations_.addComponent<Renderer>(obj);
        }
        if (!obj.has<BoxCollider>()) {
            if (ImGui::Selectable("  Box Collider")) mutations_.addComponent<BoxCollider>(obj);
        }
        if (obj.has<Transform>() && obj.has<Camera>() && obj.has<Renderer>() && obj.has<BoxCollider>()) {
            ImGui::TextDisabled("  All components added");
        }
        ImGui::Unindent();
    }
};
