#pragma once
#include <imgui.h>
#include <optional>
#include "../../../business/ObjectSelection.hpp"
#include "../../../business/scene_editing/SceneMutations.hpp"
#include "../ui_components/inspector_panel/BoxColliderWidget.hpp"
#include "../ui_components/inspector_panel/CameraWidget.hpp"
#include "../ui_components/inspector_panel/RendererWidget.hpp"
#include "../ui_components/inspector_panel/TransformWidget.hpp"
#include "ComponentContainer.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/scene/EntityManager.hpp"
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
        cameraWidget_(sceneMutations),
        transformContainer_("Transform", 4),
        cameraContainer_("Camera", 5),
        rendererContainer_("Renderer", 7),
        boxColliderContainer_("Box Collider", 3) {}

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
            transformContainer_.drawWithContextMenu(
                    "TransformContext",
                    [&] { transformWidget_.draw(*transform); },
                    [this, entity] { sceneMutations_.removeComponent<Transform>(entity); });
        }
        ImGui::Spacing();
        if (entityManager_.hasComponent<Camera>(entity)) {
            auto* camera = entityManager_.getComponent<Camera>(entity);
            cameraContainer_.drawWithContextMenu(
                    "CameraContext",
                    [&] { cameraWidget_.draw(*camera); },
                    [this, entity] { sceneMutations_.removeComponent<Camera>(entity); });
        }
        ImGui::Spacing();
        if (entityManager_.hasComponent<Renderer>(entity)) {
            auto* renderer = entityManager_.getComponent<Renderer>(entity);
            rendererContainer_.drawWithContextMenu(
                    "RendererContext",
                    [&] { rendererWidget_.draw(*renderer, entity); },
                    [this, entity] { sceneMutations_.removeComponent<Renderer>(entity); });
        }
        ImGui::Spacing();
        if (entityManager_.hasComponent<BoxCollider>(entity)) {
            auto* boxCollider = entityManager_.getComponent<BoxCollider>(entity);
            boxColliderContainer_.drawWithContextMenu(
                    "BoxColliderContext",
                    [&] { boxColliderWidget_.draw(*boxCollider); },
                    [this, entity] { sceneMutations_.removeComponent<BoxCollider>(entity); });
        }
        ImGui::Spacing();
        drawAddComponent(entity);
    }

    void drawAddComponent(EntityHandle entity) {
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("+ Add Component", {ImGui::GetContentRegionAvail().x, 0})) {
            ImGui::OpenPopup("AddComponentMenu");
        }
        if (ImGui::BeginPopup("AddComponentMenu")) {
            if (!entityManager_.hasComponent<Transform>(entity)) {
                if (ImGui::MenuItem("Transform")) sceneMutations_.addComponent<Transform>(entity);
            }
            if (!entityManager_.hasComponent<Camera>(entity)) {
                if (ImGui::MenuItem("Camera")) sceneMutations_.addComponent<Camera>(entity);
            }
            if (!entityManager_.hasComponent<Renderer>(entity)) {
                if (ImGui::MenuItem("Renderer")) sceneMutations_.addComponent<Renderer>(entity);
            }
            if (!entityManager_.hasComponent<BoxCollider>(entity)) {
                if (ImGui::MenuItem("Box Collider")) sceneMutations_.addComponent<BoxCollider>(entity);
            }
            if (entityManager_.hasComponent<Transform>(entity) && entityManager_.hasComponent<Camera>(entity) &&
                entityManager_.hasComponent<Renderer>(entity) && entityManager_.hasComponent<BoxCollider>(entity)) {
                ImGui::TextDisabled("All components added");
            }
            ImGui::EndPopup();
        }
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
    ComponentContainer transformContainer_;
    ComponentContainer cameraContainer_;
    ComponentContainer rendererContainer_;
    ComponentContainer boxColliderContainer_;
};
