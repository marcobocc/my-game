#include "PresentationLayer.hpp"
#include "../../../engine/data/components/Renderer.hpp"
#include "../../../engine/modules/assets/AssetManager.hpp"
#include "../../../engine/modules/scene/EntityManager.hpp"
#include "../business/EditorGizmos.hpp"
#include "../business/EditorOrbitCamera.hpp"
#include "../business/ObjectSelection.hpp"
#include "../business/ObjectTransformHandle.hpp"
#include "../business/scene_editing/SceneMutations.hpp"
#include "../input/PickingSystem.hpp"
#include "../presentation/imgui/containers/AssetsExplorer.hpp"
#include "../presentation/imgui/containers/EditorMenuBar.hpp"
#include "../presentation/imgui/containers/HierarchyPanel.hpp"
#include "../presentation/imgui/containers/InspectorPanel.hpp"
#include "../presentation/imgui/containers/SceneViewToolbar.hpp"
#include "gizmos/GizmosBuilder.hpp"
#include "vulkan/EditorRenderData.hpp"
#include "vulkan/VulkanEditorBackend.hpp"

PresentationLayer::PresentationLayer(VulkanEditorBackend& renderer,
                                     EditorOrbitCamera& editorOrbitCamera,
                                     ObjectSelection& objectSelection,
                                     EditorGizmos& editorGizmos,
                                     EditorSettings& editorSettings,
                                     RendererSettings& rendererSettings,
                                     EntityManager& entityManager,
                                     GizmosRenderer& gizmosBuilder,
                                     ObjectTransformHandle& objectTransformHandle,
                                     UserInterface& userInterface,
                                     PickingSystem& pickingService,
                                     SceneMutations& sceneMutations,
                                     SceneLoader& editorWorkspace,
                                     AssetManager& assetManager) :
    renderer_(renderer),
    editorOrbitCamera_(editorOrbitCamera),
    objectSelection_(objectSelection),
    editorGizmos_(editorGizmos),
    editorSettings_(editorSettings),
    rendererSettings_(rendererSettings),
    entityManager_(entityManager),
    gizmosBuilder_(gizmosBuilder),
    objectTransformHandle_(objectTransformHandle),
    userInterface_(userInterface),
    pickingSystem_(pickingService),
    sceneMutations_(sceneMutations),
    editorWorkspace_(editorWorkspace),
    assetManager_(assetManager) {
    userInterface_.emplace<EditorMenuBar>(sceneMutations, editorWorkspace);
    userInterface_.emplace<SceneViewToolbar>(editorSettings_, editorGizmos_);
    userInterface_.emplace<HierarchyPanel>(objectSelection_, entityManager_, assetManager_, sceneMutations);
    userInterface_.emplace<AssetsExplorer>(assetManager_);
    userInterface_.emplace<InspectorPanel>(
            objectSelection_, entityManager_, assetManager_, sceneMutations, editorGizmos_);
}

void PresentationLayer::buildOutlines() {
    outlineQueue_.clear();
    auto selectedId = objectSelection_.getSelectedEntityId();
    if (selectedId.has_value()) {
        auto* renderer = entityManager_.getComponent<Renderer>(*selectedId);
        auto* transform = entityManager_.getComponent<Transform>(*selectedId);
        if (renderer && transform) {
            outlineQueue_.push_back(DrawCall{*renderer, *transform, std::to_string(*selectedId)});
        }
    }
}

void PresentationLayer::buildGizmos() {
    builtGizmoLines_.clear();
    pickingSystem_.clearHandles();

    auto selectedId = objectSelection_.getSelectedEntityId();
    if (!selectedId.has_value()) return;

    const Camera& camera = editorOrbitCamera_.getCamera();
    const Transform& cameraTransform = editorOrbitCamera_.getCameraTransform();
    auto dragState = objectTransformHandle_.getDragState();

    if (dragState.gizmoMode == GizmoType::Translation) {
        auto result = gizmosBuilder_.buildTranslationGizmo(*selectedId, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    } else if (dragState.gizmoMode == GizmoType::Rotation) {
        auto result = gizmosBuilder_.buildRotationGizmo(*selectedId, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    } else if (dragState.gizmoMode == GizmoType::Scale) {
        auto result = gizmosBuilder_.buildScaleGizmo(*selectedId, camera, cameraTransform);
        builtGizmoLines_.insert(builtGizmoLines_.end(), result.visualization.begin(), result.visualization.end());
        for (const auto& h: result.pickingHandles) {
            pickingSystem_.registerHandle(h);
        }
    }

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
}

void PresentationLayer::render(const EntityManager& entityManager,
                               float gridScale,
                               const std::vector<DrawCall>& outlineQueue) {
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

    auto cameras = entityManager.query<Camera, Transform>();
    for (auto& [entity, camera, transform]: cameras) {
        if (camera->renderTarget.isValid())
            renderer_.renderFrame(
                    EditorRenderData(*camera, *transform, drawQueue, outlineQueue_, builtGizmoLines_, gridScale, true));
    }

    renderer_.renderFrame(EditorRenderData(editorOrbitCamera_.getCamera(),
                                           editorOrbitCamera_.getCameraTransform(),
                                           drawQueue,
                                           outlineQueue_,
                                           builtGizmoLines_,
                                           gridScale,
                                           false));
}
