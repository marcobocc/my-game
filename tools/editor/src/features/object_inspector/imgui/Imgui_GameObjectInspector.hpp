#pragma once
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetStore.hpp"
#include "../../../services/common_editing/RuntimeScene.hpp"
#include "../../../styling/ImguiStyling.hpp"
#include "components/Imgui_BehaviourScriptWidget.hpp"
#include "components/Imgui_BoxColliderWidget.hpp"
#include "components/Imgui_CameraWidget.hpp"
#include "components/Imgui_LightWidget.hpp"
#include "components/Imgui_ParticleEmitterWidget.hpp"
#include "components/Imgui_RendererWidget.hpp"
#include "components/Imgui_TransformWidget.hpp"
#include "modules/scene/components/BehaviourScript.hpp"
#include "modules/scene/components/BoxCollider.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/ParticleEmitter.hpp"
#include "modules/scene/components/Renderer.hpp"
#include "modules/scene/components/Transform.hpp"

class Imgui_GameObjectInspector {
public:
    Imgui_GameObjectInspector(EditorSelection& editorSelection,
                              AssetStore& assetStore,
                              RuntimeScene& scene,
                              EditorGizmos& debugViz) :
        editorSelection_(editorSelection),
        assetStore_(assetStore),
        scene_(scene),
        debugViz_(debugViz),
        transformWidget_(scene),
        rendererWidget_(assetStore, scene, debugViz),
        boxColliderWidget_(scene),
        cameraWidget_(scene),
        lightWidget_(scene),
        particleEmitterWidget_(scene) {}

    void draw(EntityHandle entity) {
        transformWidget_.setCurrentObjectId(entity);
        cameraWidget_.setCurrentObjectId(entity);
        boxColliderWidget_.setCurrentObjectId(entity);
        lightWidget_.setCurrentObjectId(entity);
        particleEmitterWidget_.setCurrentObjectId(entity);
        drawObject(entity);
    }

private:
    EditorSelection& editorSelection_;
    AssetStore& assetStore_;
    RuntimeScene& scene_;
    EditorGizmos& debugViz_;
    Imgui_TransformWidget transformWidget_;
    Imgui_RendererWidget rendererWidget_;
    Imgui_BoxColliderWidget boxColliderWidget_;
    Imgui_CameraWidget cameraWidget_;
    Imgui_LightWidget lightWidget_;
    Imgui_ParticleEmitterWidget particleEmitterWidget_;
    std::vector<std::unique_ptr<Imgui_BehaviourScriptWidget>> behaviourScriptWidgets_;

    void drawObject(EntityHandle entity) {
        RuntimeGameObject obj = scene_.getObject(entity);
        if (const auto* transform = obj.getComponent<Transform>()) {
            transformWidget_.setComponent(*transform);
            transformWidget_.draw("TransformContext", nullptr);
        }
        if (const auto* camera = obj.getComponent<Camera>()) {
            cameraWidget_.setComponent(*camera);
            cameraWidget_.draw("CameraContext", [this, entity] { scene_.getObject(entity).removeComponent<Camera>(); });
        }
        if (const auto* renderer = obj.getComponent<Renderer>()) {
            rendererWidget_.setComponent(*renderer, entity);
            rendererWidget_.draw("RendererContext",
                                 [this, entity] { scene_.getObject(entity).removeComponent<Renderer>(); });
        }
        if (const auto* boxCollider = obj.getComponent<BoxCollider>()) {
            boxColliderWidget_.setComponent(*boxCollider);
            boxColliderWidget_.draw("BoxColliderContext",
                                    [this, entity] { scene_.getObject(entity).removeComponent<BoxCollider>(); });
        }
        if (const auto* light = obj.getComponent<Light>()) {
            lightWidget_.setComponent(*light);
            lightWidget_.draw("LightContext", [this, entity] { scene_.getObject(entity).removeComponent<Light>(); });
        }
        if (const auto* particleEmitter = obj.getComponent<ParticleEmitter>()) {
            particleEmitterWidget_.setComponent(*particleEmitter);
            particleEmitterWidget_.draw("ParticleEmitterContext", [this, entity] {
                scene_.getObject(entity).removeComponent<ParticleEmitter>();
            });
        }
        auto scripts = scene_.getObject(entity).getComponents<BehaviourScript>();
        while (behaviourScriptWidgets_.size() < scripts.size())
            behaviourScriptWidgets_.emplace_back(std::make_unique<Imgui_BehaviourScriptWidget>(
                    scene_, "BehaviourScript_" + std::to_string(behaviourScriptWidgets_.size())));
        for (size_t i = 0; i < scripts.size(); ++i) {
            behaviourScriptWidgets_[i]->setCurrentObjectId(entity);
            behaviourScriptWidgets_[i]->setComponent(*scripts[i], i);
            std::string contextId = "BehaviourScriptContext_" + std::to_string(i);
            behaviourScriptWidgets_[i]->draw(contextId.c_str(), [this, entity, i] {
                scene_.getObject(entity).removeComponentAt<BehaviourScript>(i);
            });
        }
        drawAddComponent(entity);
    }

    void drawAddComponent(EntityHandle entity) const {
        ImGui::Separator();
        ImGui::Dummy({0.0f, 4.0f});
        if (ImGui::Button("+ Add Component", {ImGui::GetContentRegionAvail().x, 0})) {
            ImGui::OpenPopup("AddComponentMenu");
        }
        RuntimeGameObject obj = scene_.getObject(entity);
        ImguiStyling::withPopup("AddComponentMenu", [this, entity, &obj] {
            if (!obj.getComponent<Transform>()) {
                if (ImGui::MenuItem("Transform")) scene_.getObject(entity).addComponent<Transform>(Transform{});
            }
            if (!obj.getComponent<Camera>()) {
                if (ImGui::MenuItem("Camera")) scene_.getObject(entity).addComponent<Camera>(Camera{});
            }
            if (!obj.getComponent<Renderer>()) {
                if (ImGui::MenuItem("Renderer")) scene_.getObject(entity).addComponent<Renderer>(Renderer{});
            }
            if (!obj.getComponent<BoxCollider>()) {
                if (ImGui::MenuItem("Box Collider")) scene_.getObject(entity).addComponent<BoxCollider>(BoxCollider{});
            }
            if (!obj.getComponent<Light>()) {
                if (ImGui::MenuItem("Light")) scene_.getObject(entity).addComponent<Light>(Light{});
            }
            if (!obj.getComponent<ParticleEmitter>()) {
                if (ImGui::MenuItem("Particle Emitter"))
                    scene_.getObject(entity).addComponent<ParticleEmitter>(ParticleEmitter{});
            }
            if (ImGui::MenuItem("Behaviour Script"))
                scene_.getObject(entity).addComponent<BehaviourScript>(BehaviourScript{});
            if (obj.getComponent<Transform>() && obj.getComponent<Camera>() && obj.getComponent<Renderer>() &&
                obj.getComponent<BoxCollider>() && obj.getComponent<Light>() && obj.getComponent<ParticleEmitter>()) {
                ImGui::TextDisabled("All single-instance components added");
            }
        });
    }
};
