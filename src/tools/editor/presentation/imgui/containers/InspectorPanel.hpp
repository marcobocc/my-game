#pragma once
#include <imgui.h>
#include <optional>
#include "../../../business/EditorGizmos.hpp"
#include "../../../business/ObjectSelection.hpp"
#include "../../../business/SceneMutations.hpp"
#include "../ui_components/inspector_panel/BoxColliderWidget.hpp"
#include "../ui_components/inspector_panel/CameraWidget.hpp"
#include "../ui_components/inspector_panel/RendererWidget.hpp"
#include "../ui_components/inspector_panel/TransformWidget.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/scene/Scene.hpp"
#include "systems/ui/ImguiWidget.hpp"

class InspectorPanel : public ImguiWidget {
public:
    InspectorPanel(ObjectSelection& objectSelection,
                   Scene& scene,
                   AssetManager& assetManager,
                   SceneMutations& sceneMutations,
                   EditorGizmos& debugViz) :
        objectSelection_(objectSelection),
        scene_(scene),
        assetManager_(assetManager),
        sceneMutations_(sceneMutations),
        debugViz_(debugViz),
        transformWidget_(sceneMutations),
        rendererWidget_(assetManager, sceneMutations, debugViz),
        boxColliderWidget_(sceneMutations),
        cameraWidget_(sceneMutations) {}

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

        auto selectedId = objectSelection_.getSelectedObjectId();
        if (!selectedId.has_value()) {
            ImGui::TextDisabled("No object selected");
        } else {
            transformWidget_.setCurrentObjectId(*selectedId);
            drawObject(scene_.getObject(*selectedId));
        }

        ImGui::End();
    }

    static constexpr float PANEL_WIDTH_RATIO = 0.25f;

private:
    void drawObject(GameObject& obj) {
        if (obj.has<Transform>()) {
            transformWidget_.setCurrentObjectId(obj.getName());
            transformWidget_.draw(obj.get<Transform>());
            drawComponentContextMenu<Transform>("TransformContext", obj);
        }
        ImGui::Spacing();
        if (obj.has<Camera>()) {
            cameraWidget_.setCurrentObjectId(obj.getName());
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
            boxColliderWidget_.setCurrentObjectId(obj.getName());
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
                emitRemoveComponentEvent<T>(obj);
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
            if (ImGui::Selectable("  Transform")) emitAddComponentEvent("Transform", obj);
        }
        if (!obj.has<Camera>()) {
            if (ImGui::Selectable("  Camera")) emitAddComponentEvent("Camera", obj);
        }
        if (!obj.has<Renderer>()) {
            if (ImGui::Selectable("  Renderer")) emitAddComponentEvent("Renderer", obj);
        }
        if (!obj.has<BoxCollider>()) {
            if (ImGui::Selectable("  Box Collider")) emitAddComponentEvent("BoxCollider", obj);
        }
        if (obj.has<Transform>() && obj.has<Camera>() && obj.has<Renderer>() && obj.has<BoxCollider>()) {
            ImGui::TextDisabled("  All components added");
        }
        ImGui::Unindent();
    }

    template<typename T>
    void emitRemoveComponentEvent(GameObject& obj) {
        std::string componentName;
        if constexpr (std::is_same_v<T, Transform>) {
            componentName = "Transform";
        } else if constexpr (std::is_same_v<T, Camera>) {
            componentName = "Camera";
        } else if constexpr (std::is_same_v<T, Renderer>) {
            componentName = "Renderer";
        } else if constexpr (std::is_same_v<T, BoxCollider>) {
            componentName = "BoxCollider";
        }
        sceneMutations_.removeComponentByType(obj.getName(), componentName);
    }

    void emitAddComponentEvent(const std::string& componentType, GameObject& obj) {
        sceneMutations_.addComponentByType(obj.getName(), componentType);
    }

    ObjectSelection& objectSelection_;
    Scene& scene_;
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    EditorGizmos& debugViz_;
    TransformWidget transformWidget_;
    RendererWidget rendererWidget_;
    BoxColliderWidget boxColliderWidget_;
    CameraWidget cameraWidget_;
};
