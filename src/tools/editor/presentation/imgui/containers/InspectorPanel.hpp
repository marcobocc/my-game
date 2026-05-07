#pragma once
#include <imgui.h>
#include <optional>
#include "../../../business/EditorGizmos.hpp"
#include "../../../business/ObjectSelection.hpp"
#include "../../../business/scene_editing/SceneMutations.hpp"
#include "../ui_components/inspector_panel/BoxColliderWidget.hpp"
#include "../ui_components/inspector_panel/CameraWidget.hpp"
#include "../ui_components/inspector_panel/RendererWidget.hpp"
#include "../ui_components/inspector_panel/TransformWidget.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/scene/EntityManager.hpp"
#include "modules/scene/EntityMetadata.hpp"
#include "modules/ui/ImguiWidget.hpp"

class InspectorPanel : public ImguiWidget {
public:
    InspectorPanel(ObjectSelection& objectSelection,
                   EntityManager& entityManager,
                   AssetManager& assetManager,
                   SceneMutations& sceneMutations,
                   EditorGizmos& debugViz) :
        objectSelection_(objectSelection),
        entityManager_(entityManager),
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

        auto selectedId = objectSelection_.getSelectedEntityId();
        if (!selectedId.has_value()) {
            ImGui::TextDisabled("No object selected");
        } else {
            EntityHandle entity = *selectedId;
            transformWidget_.setCurrentObjectId(entity);
            cameraWidget_.setCurrentObjectId(entity);
            boxColliderWidget_.setCurrentObjectId(entity);
            drawObject(entity);
        }

        ImGui::End();
    }

    static constexpr float PANEL_WIDTH_RATIO = 0.25f;

private:
    void drawObject(EntityHandle entity) {
        if (entityManager_.hasComponent<Transform>(entity)) {
            auto* transform = entityManager_.getComponent<Transform>(entity);
            transformWidget_.draw(*transform);
            drawComponentContextMenu<Transform>("TransformContext", entity);
        }
        ImGui::Spacing();
        if (entityManager_.hasComponent<Camera>(entity)) {
            auto* camera = entityManager_.getComponent<Camera>(entity);
            cameraWidget_.draw(*camera);
            drawComponentContextMenu<Camera>("CameraContext", entity);
        }
        ImGui::Spacing();
        if (entityManager_.hasComponent<Renderer>(entity)) {
            auto* renderer = entityManager_.getComponent<Renderer>(entity);
            rendererWidget_.draw(*renderer, entity);
            drawComponentContextMenu<Renderer>("RendererContext", entity);
        }
        ImGui::Spacing();
        if (entityManager_.hasComponent<BoxCollider>(entity)) {
            auto* boxCollider = entityManager_.getComponent<BoxCollider>(entity);
            boxColliderWidget_.draw(*boxCollider);
            drawComponentContextMenu<BoxCollider>("BoxColliderContext", entity);
        }
        ImGui::Spacing();
        drawAddComponent(entity);
    }

    template<typename T>
    void drawComponentContextMenu(const char* contextId, EntityHandle entity) {
        if (ImGui::BeginPopupContextItem(contextId)) {
            if (ImGui::MenuItem("Remove")) {
                emitRemoveComponentEvent<T>(entity);
            }
            ImGui::EndPopup();
        }
    }

    void drawAddComponent(EntityHandle entity) {
        ImGui::Separator();
        ImGui::Spacing();
        if (!ImGui::CollapsingHeader("Add Component")) return;

        ImGui::Indent();
        if (!entityManager_.hasComponent<Transform>(entity)) {
            if (ImGui::Selectable("  Transform")) emitAddComponentEvent("Transform", entity);
        }
        if (!entityManager_.hasComponent<Camera>(entity)) {
            if (ImGui::Selectable("  Camera")) emitAddComponentEvent("Camera", entity);
        }
        if (!entityManager_.hasComponent<Renderer>(entity)) {
            if (ImGui::Selectable("  Renderer")) emitAddComponentEvent("Renderer", entity);
        }
        if (!entityManager_.hasComponent<BoxCollider>(entity)) {
            if (ImGui::Selectable("  Box Collider")) emitAddComponentEvent("BoxCollider", entity);
        }
        if (entityManager_.hasComponent<Transform>(entity) && entityManager_.hasComponent<Camera>(entity) &&
            entityManager_.hasComponent<Renderer>(entity) && entityManager_.hasComponent<BoxCollider>(entity)) {
            ImGui::TextDisabled("  All components added");
        }
        ImGui::Unindent();
    }

    template<typename T>
    void emitRemoveComponentEvent(EntityHandle entity) {
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
        sceneMutations_.removeComponentByType(entity, componentName);
    }

    void emitAddComponentEvent(const std::string& componentType, EntityHandle entity) {
        sceneMutations_.addComponentByType(entity, componentType);
    }

    ObjectSelection& objectSelection_;
    EntityManager& entityManager_;
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    EditorGizmos& debugViz_;
    TransformWidget transformWidget_;
    RendererWidget rendererWidget_;
    BoxColliderWidget boxColliderWidget_;
    CameraWidget cameraWidget_;
};
