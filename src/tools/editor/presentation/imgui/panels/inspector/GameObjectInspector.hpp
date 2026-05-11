#pragma once
#include <imgui.h>
#include "../../../../business/ObjectSelection.hpp"
#include "../../../../business/scene_editing/SceneMutations.hpp"
#include "../../ImguiStyling.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "game_object/BoxColliderWidget.hpp"
#include "game_object/CameraWidget.hpp"
#include "game_object/RendererWidget.hpp"
#include "game_object/TransformWidget.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/scene/EntityManager.hpp"

class GameObjectInspector {
public:
    GameObjectInspector(ObjectSelection& objectSelection,
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

    void draw(EntityHandle entity) {
        transformWidget_.setCurrentObjectId(entity);
        cameraWidget_.setCurrentObjectId(entity);
        boxColliderWidget_.setCurrentObjectId(entity);
        drawObject(entity);
    }

private:
    ObjectSelection& objectSelection_;
    EntityManager& entityManager_;
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    EditorGizmos& debugViz_;
    TransformWidget transformWidget_;
    RendererWidget rendererWidget_;
    BoxColliderWidget boxColliderWidget_;
    CameraWidget cameraWidget_;

    void drawObject(EntityHandle entity) {
        if (entityManager_.hasComponent<Transform>(entity)) {
            auto* transform = entityManager_.getComponent<Transform>(entity);
            transformWidget_.setComponent(*transform);
            transformWidget_.draw("TransformContext",
                                  [this, entity] { sceneMutations_.removeComponent<Transform>(entity); });
        }
        if (entityManager_.hasComponent<Camera>(entity)) {
            auto* camera = entityManager_.getComponent<Camera>(entity);
            cameraWidget_.setComponent(*camera);
            cameraWidget_.draw("CameraContext", [this, entity] { sceneMutations_.removeComponent<Camera>(entity); });
        }
        if (entityManager_.hasComponent<Renderer>(entity)) {
            auto* renderer = entityManager_.getComponent<Renderer>(entity);
            rendererWidget_.setComponent(*renderer, entity);
            rendererWidget_.draw("RendererContext",
                                 [this, entity] { sceneMutations_.removeComponent<Renderer>(entity); });
        }
        if (entityManager_.hasComponent<BoxCollider>(entity)) {
            auto* boxCollider = entityManager_.getComponent<BoxCollider>(entity);
            boxColliderWidget_.setComponent(*boxCollider);
            boxColliderWidget_.draw("BoxColliderContext",
                                    [this, entity] { sceneMutations_.removeComponent<BoxCollider>(entity); });
        }
        drawAddComponent(entity);
    }

    void drawAddComponent(EntityHandle entity) const {
        ImGui::Separator();
        ImGui::Dummy({0.0f, 4.0f});
        if (ImGui::Button("+ Add Component", {ImGui::GetContentRegionAvail().x, 0})) {
            ImGui::OpenPopup("AddComponentMenu");
        }
        ImguiStyling::withPopup("AddComponentMenu", [&] {
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
        });
    }
};
