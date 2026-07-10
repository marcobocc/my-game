#pragma once
#include <vector>
#include "../../../../runtime/src/graphics/debug/DebugDraw.hpp"
#include "animation/AnimationSystem.hpp"
#include "core/scene/World.hpp"
#include "graphics/vulkan/passes/VulkanGizmoPass.hpp"

class VulkanBackend;
class EditorCamera;
class EditorSelection;
class EditorGizmos;
class GizmoBuilder;
class ObjectTransformHandle;
class PickingSystem;
class ImguiRoot;
class SimHUDRoot;
class WelcomeRoot;
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
                   SimHUDRoot& simHUDRoot,
                   WelcomeRoot& welcomeRoot);

    void setSimMode(bool enabled);
    void setWelcomeMode(bool enabled);
    void setAnimationSystem(AnimationSystem* animSystem) { animationSystem_ = animSystem; }
    void setPlayModeDebugDraw(DebugDraw* debugDraw) { playModeDebugDraw_ = debugDraw; }
    void render(const World& entityManager, float gridScale, float deltaTime = 0.0f);
    void renderWelcome();

private:
    void buildGizmos(const World& world, DebugDraw& out);
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
    WelcomeRoot& welcomeRoot_;
    AnimationSystem* animationSystem_ = nullptr;
    DebugDraw* playModeDebugDraw_ = nullptr;
    DebugDraw editorDebugDraw_;
    bool simMode_ = false;
    float deltaTime_ = 0.0f;
    std::vector<DrawCall> outlineQueue_;
    std::vector<VulkanGizmoPass::GizmoVertex> builtOverlayGizmoLines_;
};
