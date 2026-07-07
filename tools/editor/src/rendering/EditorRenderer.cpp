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
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/debug/DebugDraw.hpp"
#include "modules/rendering/GameRenderData.hpp"
#include "modules/scene/TransformUtils.hpp"
#include "modules/scene/World.hpp"
#include "modules/scene/components/ParticleEmitter.hpp"
#include "modules/scene/components/Renderer.hpp"
#include "modules/scene/components/TextComponent.hpp"

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
            const glm::mat4* skinBones = nullptr;
            if (animationSystem_ && animationSystem_->hasSkinning(selectedId))
                skinBones = animationSystem_->getSkinningMatrices(selectedId).bones;
            outlineQueue_.push_back(DrawCall{*renderer,
                                             *transform,
                                             TransformUtils::resolveWorldMatrix(*transform, entityManager_),
                                             std::to_string(selectedId),
                                             skinBones});
        }
    }
}

void EditorRenderer::buildGizmos(const World& world, DebugDraw& out) {
    builtOverlayGizmoLines_.clear();
    pickingSystem_.clearHandles();

    for (EntityHandle objectId: editorGizmos_.getObjectsWithAABBEnabled())
        gizmosBuilder_.buildGizmoObjectAABB(out, objectId, world, {0.0f, 1.0f, 0.0f});

    for (EntityHandle objectId: editorGizmos_.getObjectsWithBoundingSpheresEnabled())
        gizmosBuilder_.buildGizmoObjectBoundingSphere(out, objectId, world, {1.0f, 1.0f, 0.0f});

    for (EntityHandle objectId: editorGizmos_.getObjectsWithBoxColliderEnabled())
        gizmosBuilder_.buildGizmoObjectBoxCollider(out, objectId, world, {0.0f, 1.0f, 0.5f});

    for (EntityHandle objectId: editorGizmos_.getObjectsWithCapsuleColliderEnabled())
        gizmosBuilder_.buildGizmoObjectCapsuleCollider(out, objectId, world, {0.0f, 0.8f, 1.0f});

    if (editorGizmos_.bvhEnabled()) gizmosBuilder_.buildGizmoBVH(out, world, {1.0f, 1.0f, 0.0f});

    const auto& selectedIds = editorSelection_.getSelectedEntityIds();
    if (selectedIds.empty()) return;

    bool anyHasTransform = std::ranges::any_of(selectedIds, [&](EntityHandle id) {
        auto* actor = entityManager_.getActor(id);
        return actor && actor->getComponent<Transform>() != nullptr;
    });
    if (!anyHasTransform) return;

    glm::vec3 pivot = objectTransformHandle_.computeGroupPivot(selectedIds);
    const Camera& camera = editorOrbitCamera_.getCamera();
    const Transform& cameraTransform = editorOrbitCamera_.getCameraTransform();
    auto dragState = objectTransformHandle_.getDragState();

    if (dragState.gizmoMode == GizmoType::Translation) {
        auto result = objectTransformHandle_.buildTranslationGizmo(pivot, camera, cameraTransform);
        builtOverlayGizmoLines_.insert(
                builtOverlayGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles)
            pickingSystem_.registerHandle(h);
    } else if (dragState.gizmoMode == GizmoType::Rotation) {
        auto result = objectTransformHandle_.buildRotationGizmo(pivot, camera, cameraTransform);
        builtOverlayGizmoLines_.insert(
                builtOverlayGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles)
            pickingSystem_.registerHandle(h);
    } else if (dragState.gizmoMode == GizmoType::Scale) {
        auto result = objectTransformHandle_.buildScaleGizmo(pivot, camera, cameraTransform);
        builtOverlayGizmoLines_.insert(
                builtOverlayGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles)
            pickingSystem_.registerHandle(h);
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
        const glm::mat4* skinBones = nullptr;
        if (animationSystem_ && animationSystem_->hasSkinning(entity))
            skinBones = animationSystem_->getSkinningMatrices(entity).bones;
        drawQueue.push_back(DrawCall{*renderer,
                                     *transform,
                                     TransformUtils::resolveWorldMatrix(*transform, entityManager),
                                     std::to_string(entity),
                                     skinBones});
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

    std::vector<TextDrawCall> textQueue;
    auto textQuery = entityManager.query<TextComponent, Transform>();
    for (auto& [entity, textComp, transform]: textQuery) {
        if (!textComp->visible || textComp->text.empty()) continue;
        textQueue.push_back({textComp->text,
                             textComp->fontName.empty() ? DEFAULT_FONT : textComp->fontName,
                             TransformUtils::resolveWorldMatrix(*transform, entityManager)[3],
                             textComp->color,
                             textComp->fontSize,
                             textComp->billboard,
                             textComp->alignment});
    }

    if (simMode_) {
        static const std::vector<DrawCall> emptyOutlines;
        static const std::vector<VulkanGizmoPass::GizmoVertex> emptyOverlay;
        if (playModeDebugDraw_) buildGizmos(entityManager, *playModeDebugDraw_);
        const std::vector<VulkanGizmoPass::GizmoVertex>& debugLines =
                playModeDebugDraw_ ? playModeDebugDraw_->getVertices() : emptyOverlay;

        const Actor* camActor = entityManager.getActiveCamera();
        if (camActor) {
            const auto* camera = camActor->getComponent<Camera>();
            const auto* transform = camActor->getComponent<Transform>();
            if (camera && transform) {
                renderer_.renderFrame(EditorRenderData(*camera,
                                                       *transform,
                                                       drawQueue,
                                                       emptyOutlines,
                                                       debugLines,
                                                       emptyOverlay,
                                                       activeLights,
                                                       particleEmitters,
                                                       textQueue,
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
                                               debugLines,
                                               emptyOverlay,
                                               activeLights,
                                               particleEmitters,
                                               textQueue,
                                               0.0f,
                                               false));
        return;
    }

    buildOutlines();
    editorDebugDraw_.flush();
    buildGizmos(entityManager, editorDebugDraw_);

    const std::vector<VulkanGizmoPass::GizmoVertex>& editorGizmoLines = editorDebugDraw_.getVertices();

    auto cameras = entityManager.query<Camera, Transform>();
    for (auto& [entity, camera, transform]: cameras) {
        if (camera->renderTarget.isValid())
            renderer_.renderFrame(EditorRenderData(*camera,
                                                   *transform,
                                                   drawQueue,
                                                   outlineQueue_,
                                                   editorGizmoLines,
                                                   builtOverlayGizmoLines_,
                                                   activeLights,
                                                   particleEmitters,
                                                   textQueue,
                                                   gridScale,
                                                   true));
    }

    renderer_.renderFrame(EditorRenderData(editorOrbitCamera_.getCamera(),
                                           editorOrbitCamera_.getCameraTransform(),
                                           drawQueue,
                                           outlineQueue_,
                                           editorGizmoLines,
                                           builtOverlayGizmoLines_,
                                           activeLights,
                                           particleEmitters,
                                           textQueue,
                                           gridScale,
                                           false));
}
