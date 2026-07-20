#pragma once
#include <functional>
#include <vector>
#include "../../../../runtime/src/graphics/debug/DebugDraw.hpp"
#include "../services/EditorMode.hpp"
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
class UISystem;
class GameWindow;
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
                   WelcomeRoot& welcomeRoot,
                   EditorModeService& editorMode);

    void setSimMode(bool enabled);
    void setWelcomeMode(bool enabled);
    void setAnimationSystem(AnimationSystem* animSystem) { animationSystem_ = animSystem; }
    void setPlayModeDebugDraw(DebugDraw* debugDraw) { playModeDebugDraw_ = debugDraw; }
    void setPlayModeUISource(const UISystem* uiSystem, const GameWindow* window) {
        playModeUISystem_ = uiSystem;
        playModeUIWindow_ = window;
    }
    // Invoked each frame (editor/non-sim mode only) after gizmo lines are built, so callers can
    // inject additional debug draws (e.g. terrain brush cursor) into that frame's output.
    void setDebugOverlayCallback(std::function<void(DebugDraw&)> callback) {
        debugOverlayCallback_ = std::move(callback);
    }
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
    EditorModeService& editorMode_;
    AnimationSystem* animationSystem_ = nullptr;
    DebugDraw* playModeDebugDraw_ = nullptr;
    const UISystem* playModeUISystem_ = nullptr;
    const GameWindow* playModeUIWindow_ = nullptr;
    std::function<void(DebugDraw&)> debugOverlayCallback_;
    DebugDraw editorDebugDraw_;
    bool simMode_ = false;
    float deltaTime_ = 0.0f;
    std::vector<DrawCall> outlineQueue_;
    std::vector<VulkanGizmoPass::GizmoVertex> builtOverlayGizmoLines_;
};
