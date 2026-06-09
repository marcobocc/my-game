#include "EditorRenderer.hpp"
#include "../features/ImguiRoot.hpp"
#include "../features/scene_viewport/editor_camera/EditorCamera.hpp"
#include "../features/scene_viewport/gizmos/EditorGizmos.hpp"
#include "../features/scene_viewport/gizmos/GizmoBuilder.hpp"
#include "../features/scene_viewport/picking/PickingSystem.hpp"
#include "../features/scene_viewport/transform_handle/ObjectTransformHandle.hpp"
#include "../services/EditorSelection.hpp"
#include "EditorRenderData.hpp"
#include "VulkanBackend.hpp"
#include "modules/rendering/GameRenderData.hpp"
#include "modules/scene/World.hpp"
#include "modules/scene/components/ParticleEmitter.hpp"
#include "modules/scene/components/Renderer.hpp"

EditorRenderer::EditorRenderer(VulkanBackend& renderer,
                               EditorCamera& editorOrbitCamera,
                               EditorSelection& editorSelection,
                               EditorGizmos& editorGizmos,
                               World& entityManager,
                               GizmoBuilder& gizmosBuilder,
                               ObjectTransformHandle& objectTransformHandle,
                               PickingSystem& pickingService,
                               ImguiRoot& imguiRoot,
                               SimHUDRoot& simHUDRoot,
                               WelcomeRoot& welcomeRoot) :
    renderer_(renderer),
    editorOrbitCamera_(editorOrbitCamera),
    editorSelection_(editorSelection),
    editorGizmos_(editorGizmos),
    entityManager_(entityManager),
    gizmosBuilder_(gizmosBuilder),
    objectTransformHandle_(objectTransformHandle),
    pickingSystem_(pickingService),
    imguiRoot_(imguiRoot),
    simHUDRoot_(simHUDRoot),
    welcomeRoot_(welcomeRoot) {
    renderer_.setDrawCallback([this] { welcomeRoot_.draw(); });
}

void EditorRenderer::setSimMode(bool enabled) {
    simMode_ = enabled;
    if (enabled) {
        renderer_.setDrawCallback([this] { simHUDRoot_.draw(); });
    } else {
        renderer_.setDrawCallback([this] { imguiRoot_.draw(); });
    }
}

void EditorRenderer::setWelcomeMode(bool enabled) {
    if (enabled) {
        renderer_.setDrawCallback([this] { welcomeRoot_.draw(); });
    } else {
        renderer_.setDrawCallback([this] { imguiRoot_.draw(); });
    }
}

void EditorRenderer::renderWelcome() { renderer_.renderUIOnly(); }

void EditorRenderer::buildOutlines() {
    outlineQueue_.clear();
    for (EntityHandle selectedId: editorSelection_.getSelectedEntityIds()) {
        auto* actor = entityManager_.getActor(selectedId);
        if (!actor) continue;
        auto* renderer = actor->getComponent<Renderer>();
        auto* transform = actor->getComponent<Transform>();
        if (renderer && transform) {
            outlineQueue_.push_back(DrawCall{*renderer, *transform, std::to_string(selectedId)});
        }
    }
}

void EditorRenderer::buildGizmos() {
    builtGizmoLines_.clear();
    pickingSystem_.clearHandles();

    auto aabbEnabled = editorGizmos_.getObjectsWithAABBEnabled();
    for (const auto& objectId: aabbEnabled) {
        auto aabbGizmo = gizmosBuilder_.buildGizmoObjectAABB(objectId, {0.0f, 1.0f, 0.0f});
        builtGizmoLines_.insert(builtGizmoLines_.end(), aabbGizmo.begin(), aabbGizmo.end());
    }

    auto boundingSpheresEnabled = editorGizmos_.getObjectsWithBoundingSpheresEnabled();
    for (const auto& objectId: boundingSpheresEnabled) {
        auto sphereGizmo = gizmosBuilder_.buildGizmoObjectBoundingSphere(objectId, {1.0f, 1.0f, 0.0f});
        builtGizmoLines_.insert(builtGizmoLines_.end(), sphereGizmo.begin(), sphereGizmo.end());
    }

    if (editorGizmos_.bvhEnabled()) {
        auto bvhGizmo = gizmosBuilder_.buildGizmoBVH({1.0f, 1.0f, 0.0f});
        builtGizmoLines_.insert(builtGizmoLines_.end(), bvhGizmo.begin(), bvhGizmo.end());
    }

    const auto& selectedIds = editorSelection_.getSelectedEntityIds();
    if (selectedIds.empty()) return;

    glm::vec3 pivot = objectTransformHandle_.computeGroupPivot(selectedIds);
    const Camera& camera = editorOrbitCamera_.getCamera();
    const Transform& cameraTransform = editorOrbitCamera_.getCameraTransform();
    auto dragState = objectTransformHandle_.getDragState();

    if (dragState.gizmoMode == GizmoType::Translation) {
        auto result = objectTransformHandle_.buildTranslationGizmo(pivot, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    } else if (dragState.gizmoMode == GizmoType::Rotation) {
        auto result = objectTransformHandle_.buildRotationGizmo(pivot, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    } else if (dragState.gizmoMode == GizmoType::Scale) {
        auto result = objectTransformHandle_.buildScaleGizmo(pivot, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    }
}

void EditorRenderer::render(const World& entityManager, float gridScale, float deltaTime) {
    deltaTime_ = deltaTime;
    renderer_.setDeltaTime(deltaTime);
    std::vector<DrawCall> drawQueue;
    auto drawables = entityManager.query<Renderer, Transform>();
    for (auto& [entity, renderer, transform]: drawables) {
        if (!renderer->enabled) continue;
        if (renderer->meshName.empty() || renderer->materialName.empty()) {
            LOG4CXX_WARN(LOGGER,
                         "Entity " << entity
                                   << " has Renderer component with no mesh/material assigned, skipping draw call.");
            continue;
        }
        drawQueue.push_back(DrawCall{*renderer, *transform, std::to_string(entity)});
    }

    auto lightsQuery = entityManager.query<Light, Transform>();
    std::vector<std::pair<Light, Transform>> activeLights;
    for (const auto& [entity, light, transform]: lightsQuery) {
        activeLights.push_back({*light, *transform});
    }

    std::vector<ParticleEmitterRef> particleEmitters;
    auto emittersQuery = entityManager.query<ParticleEmitter, Transform>();
    for (auto& [entity, emitterPtr, transformPtr]: emittersQuery) {
        auto* e = const_cast<ParticleEmitter*>(emitterPtr);
        e->spawnAccumulator += deltaTime_ * e->emissionRate;
        auto toSpawn = static_cast<uint32_t>(e->spawnAccumulator);
        e->spawnAccumulator -= static_cast<float>(toSpawn);
        toSpawn = std::min(toSpawn, e->maxParticles);
        particleEmitters.push_back({e, transformPtr->position, toSpawn});
    }

    if (simMode_) {
        static const std::vector<DrawCall> emptyOutlines;
        static const std::vector<VulkanGizmoPass::GizmoVertex> emptyGizmos;
        const Actor* camActor = entityManager.getActiveCamera();
        if (camActor) {
            const auto* camera = camActor->getComponent<Camera>();
            const auto* transform = camActor->getComponent<Transform>();
            if (camera && transform) {
                renderer_.renderFrame(EditorRenderData(*camera,
                                                       *transform,
                                                       drawQueue,
                                                       emptyOutlines,
                                                       emptyGizmos,
                                                       activeLights,
                                                       particleEmitters,
                                                       0.0f,
                                                       false));
                return;
            }
        }
        Camera defaultCamera;
        Transform defaultTransform;
        defaultTransform.position = glm::vec3(0.0f, 1.0f, 0.0f);
        LOG4CXX_DEBUG(LOGGER, "No active Camera entity in simulation world — rendering with default camera at origin.");
        renderer_.renderFrame(EditorRenderData(defaultCamera,
                                               defaultTransform,
                                               drawQueue,
                                               emptyOutlines,
                                               emptyGizmos,
                                               activeLights,
                                               particleEmitters,
                                               0.0f,
                                               false));
        return;
    }

    buildOutlines();
    buildGizmos();

    auto cameras = entityManager.query<Camera, Transform>();
    for (auto& [entity, camera, transform]: cameras) {
        if (camera->renderTarget.isValid())
            renderer_.renderFrame(EditorRenderData(*camera,
                                                   *transform,
                                                   drawQueue,
                                                   outlineQueue_,
                                                   builtGizmoLines_,
                                                   activeLights,
                                                   particleEmitters,
                                                   gridScale,
                                                   true));
    }

    renderer_.renderFrame(EditorRenderData(editorOrbitCamera_.getCamera(),
                                           editorOrbitCamera_.getCameraTransform(),
                                           drawQueue,
                                           outlineQueue_,
                                           builtGizmoLines_,
                                           activeLights,
                                           particleEmitters,
                                           gridScale,
                                           false));
}
