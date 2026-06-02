#pragma once
#include <vector>
#include "modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "modules/scene/World.hpp"

class VulkanBackend;
class EditorCamera;
class EditorSelection;
class EditorGizmos;
class GizmoBuilder;
class ObjectTransformHandle;
class PickingSystem;
class ImguiRoot;
class SimHUDRoot;
struct DrawCall;

class EditorRenderer {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("EditorViewportRenderer");

public:
    EditorRenderer(VulkanBackend& renderer,
                   EditorCamera& editorOrbitCamera,
                   EditorSelection& editorSelection,
                   EditorGizmos& editorGizmos,
                   World& entityManager,
                   GizmoBuilder& gizmosBuilder,
                   ObjectTransformHandle& objectTransformHandle,
                   PickingSystem& pickingService,
                   ImguiRoot& imguiRoot,
                   SimHUDRoot& simHUDRoot);

    void setSimMode(bool enabled);
    void render(const World& entityManager, float gridScale);

private:
    void buildGizmos();
    void buildOutlines();

    VulkanBackend& renderer_;
    EditorCamera& editorOrbitCamera_;
    EditorSelection& editorSelection_;
    EditorGizmos& editorGizmos_;
    World& entityManager_;
    GizmoBuilder& gizmosBuilder_;
    ObjectTransformHandle& objectTransformHandle_;
    PickingSystem& pickingSystem_;
    ImguiRoot& imguiRoot_;
    SimHUDRoot& simHUDRoot_;
    bool simMode_ = false;
    std::vector<DrawCall> outlineQueue_;
    std::vector<VulkanGizmoPass::GizmoVertex> builtGizmoLines_;
};
