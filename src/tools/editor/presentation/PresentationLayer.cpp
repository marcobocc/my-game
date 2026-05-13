#include "PresentationLayer.hpp"
#include "../../../engine/modules/scene/EntityManager.hpp"
#include "../../../engine/modules/scene/components/Renderer.hpp"
#include "../business/EditorCamera.hpp"
#include "../business/EditorGizmos.hpp"
#include "../business/ObjectSelection.hpp"
#include "../business/ObjectTransformHandle.hpp"
#include "../business/asset_editing/EditorAssetRepository.hpp"
#include "../business/asset_editing/MaterialMutations.hpp"
#include "../business/scene_editing/SceneMutations.hpp"
#include "../input/PickingSystem.hpp"
#include "../presentation/imgui/toolbars/ApplicationMenuBar.hpp"
#include "../presentation/imgui/toolbars/SceneViewToolbar.hpp"
#include "gizmos/GizmosBuilder.hpp"
#include "imgui/panels/assets/AssetsPanel.hpp"
#include "imgui/panels/hierarchy/HierarchyPanel.hpp"
#include "imgui/panels/inspector/InspectorPanel.hpp"
#include "imgui/panels/viewport/EditorSceneViewport.hpp"
#include "vulkan/EditorRenderData.hpp"
#include "vulkan/VulkanEditorBackend.hpp"

PresentationLayer::PresentationLayer(VulkanEditorBackend& renderer,
                                     EditorCamera& editorOrbitCamera,
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
                                     EditorAssetRepository& assetRepository,
                                     MaterialMutations& materialMutations,
                                     SceneLoader& editorWorkspace,
                                     GameWindow& window,
                                     GameEngine& engine,
                                     UndoHistory& undoHistory) :
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
    assetRepository_(assetRepository),
    editorWorkspace_(editorWorkspace),
    engine_(engine),
    window_(window) {
    userInterface_.emplace<ApplicationMenuBar>(sceneMutations, editorWorkspace, undoHistory, objectSelection_);
    userInterface_.emplace<SceneViewToolbar>(editorSettings_, editorGizmos_);
    userInterface_.emplace<HierarchyPanel>(assetRepository_, objectSelection_, entityManager_, sceneMutations, engine_);
    userInterface_.emplace<AssetsPanel>(assetRepository_, objectSelection_, materialMutations);
    userInterface_.emplace<InspectorPanel>(
            objectSelection_, entityManager_, assetRepository_, sceneMutations, materialMutations, editorGizmos_);
    userInterface_.emplace<EditorSceneViewport>(
            assetRepository_,
            sceneMutations,
            objectSelection_,
            entityManager_,
            pickingService,
            window_,
            editorOrbitCamera,
            [&sceneMutations](uint32_t lod) { sceneMutations.createObject(primitives::sphere(lod)); });
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
