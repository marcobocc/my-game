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
#include "modules/scene/EntityStore.hpp"
#include "modules/scene/components/Renderer.hpp"

EditorRenderer::EditorRenderer(VulkanBackend& renderer,
                               EditorCamera& editorOrbitCamera,
                               EditorSelection& editorSelection,
                               EditorGizmos& editorGizmos,
                               EntityManager& entityManager,
                               GizmoBuilder& gizmosBuilder,
                               ObjectTransformHandle& objectTransformHandle,
                               PickingSystem& pickingService,
                               ImguiRoot& imguiRoot) :
    renderer_(renderer),
    editorOrbitCamera_(editorOrbitCamera),
    editorSelection_(editorSelection),
    editorGizmos_(editorGizmos),
    entityManager_(entityManager),
    gizmosBuilder_(gizmosBuilder),
    objectTransformHandle_(objectTransformHandle),
    pickingSystem_(pickingService) {
    renderer_.setDrawCallback([&] { imguiRoot.draw(); });
}

void EditorRenderer::buildOutlines() {
    outlineQueue_.clear();
    for (EntityHandle selectedId: editorSelection_.getSelectedEntityIds()) {
        auto* renderer = entityManager_.getComponent<Renderer>(selectedId);
        auto* transform = entityManager_.getComponent<Transform>(selectedId);
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
    if (selectedIds.size() != 1) return;
    EntityHandle selectedId = selectedIds[0];

    const Camera& camera = editorOrbitCamera_.getCamera();
    const Transform& cameraTransform = editorOrbitCamera_.getCameraTransform();
    auto dragState = objectTransformHandle_.getDragState();

    if (dragState.gizmoMode == GizmoType::Translation) {
        auto result = objectTransformHandle_.buildTranslationGizmo(selectedId, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    } else if (dragState.gizmoMode == GizmoType::Rotation) {
        auto result = objectTransformHandle_.buildRotationGizmo(selectedId, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    } else if (dragState.gizmoMode == GizmoType::Scale) {
        auto result = objectTransformHandle_.buildScaleGizmo(selectedId, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    }
}

void EditorRenderer::render(const EntityManager& entityManager, float gridScale) {
    buildOutlines();
    buildGizmos();

    std::vector<DrawCall> drawQueue;
    auto drawables = entityManager.query<Renderer, Transform>();
    for (auto& [entity, renderer, transform]: drawables) {
        if (!renderer->enabled) continue;
        if (renderer->meshName.empty()) {
            LOG4CXX_WARN(LOGGER,
                         "Entity " << entity << " has Renderer component with no mesh assigned, skipping draw call.");
            continue;
        }
        drawQueue.push_back(DrawCall{*renderer, *transform, std::to_string(entity)});
    }

    auto lightsQuery = entityManager.query<Light, Transform>();
    std::vector<std::pair<Light, Transform>> activeLights;
    for (const auto& [entity, light, transform]: lightsQuery) {
        activeLights.push_back({*light, *transform});
    }

    auto cameras = entityManager.query<Camera, Transform>();
    for (auto& [entity, camera, transform]: cameras) {
        if (camera->renderTarget.isValid())
            renderer_.renderFrame(EditorRenderData(
                    *camera, *transform, drawQueue, outlineQueue_, builtGizmoLines_, activeLights, gridScale, true));
    }

    renderer_.renderFrame(EditorRenderData(editorOrbitCamera_.getCamera(),
                                           editorOrbitCamera_.getCameraTransform(),
                                           drawQueue,
                                           outlineQueue_,
                                           builtGizmoLines_,
                                           activeLights,
                                           gridScale,
                                           false));
}
