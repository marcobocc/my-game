#pragma once
#include <imgui.h>
#include <optional>
#include "../../../controller/SceneMutationsController.hpp"
#include "../../../state/EditorState.hpp"
#include "../game_components/BoxColliderWidget.hpp"
#include "../game_components/CameraWidget.hpp"
#include "../game_components/RendererWidget.hpp"
#include "../game_components/TransformWidget.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/scene/Scene.hpp"
#include "systems/ui/ImguiWidget.hpp"

class InspectorPanel : public ImguiWidget {
public:
    using AABBToggleCallback = RendererWidget::AABBToggleCallback;
    using BoundingSphereToggleCallback = RendererWidget::BoundingSphereToggleCallback;

    InspectorPanel(EditorState& editorState,
                   Scene& scene,
                   AssetManager& assetManager,
                   SceneMutationsController& mutations,
                   AABBToggleCallback aabbToggle = nullptr,
                   BoundingSphereToggleCallback sphereToggle = nullptr) :
        editorState_(editorState),
        scene_(scene),
        assetManager_(assetManager),
        mutations_(mutations),
        transformWidget_(mutations),
        rendererWidget_(assetManager, mutations, aabbToggle, sphereToggle),
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

        if (!editorState_.getSelectedObject().has_value()) {
            ImGui::TextDisabled("No object selected");
        } else {
            drawObject(scene_.getObject(*editorState_.getSelectedObject()));
        }

        ImGui::End();
    }

    static constexpr float PANEL_WIDTH_RATIO = 0.25f;

private:
    EditorState& editorState_;
    Scene& scene_;
    AssetManager& assetManager_;
    SceneMutationsController& mutations_;
    TransformWidget transformWidget_;
    RendererWidget rendererWidget_;
    BoxColliderWidget boxColliderWidget_;
    CameraWidget cameraWidget_;

    void drawObject(GameObject& obj) {
        if (obj.has<Transform>()) {
            transformWidget_.draw(obj.get<Transform>());
            drawComponentContextMenu<Transform>("TransformContext", obj);
        }
        ImGui::Spacing();
        if (obj.has<Camera>()) {
            cameraWidget_.draw(obj.get<Camera>());
            drawComponentContextMenu<Camera>("CameraContext", obj);
        }
        ImGui::Spacing();
        if (obj.has<Renderer>()) {
            rendererWidget_.draw(obj.get<Renderer>(), obj.getName());
            drawComponentContextMenu<Renderer>("RendererContext", obj);
        }
        ImGui::Spacing();
        if (obj.has<BoxCollider>()) {
            boxColliderWidget_.draw(obj.get<BoxCollider>());
            drawComponentContextMenu<BoxCollider>("BoxColliderContext", obj);
        }
        ImGui::Spacing();
        drawAddComponent(obj);
    }

    template<typename T>
    void drawComponentContextMenu(const char* contextId, GameObject& obj) {
        if (ImGui::BeginPopupContextItem(contextId)) {
            if (ImGui::MenuItem("Remove")) {
                printf("Remove context menu clicked\n");
                mutations_.removeComponent<T>(obj);
            }
            ImGui::EndPopup();
        }
    }

    void drawAddComponent(GameObject& obj) {
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
