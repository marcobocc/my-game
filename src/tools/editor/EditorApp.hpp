#pragma once
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <optional>
#include <string>
#include <unordered_set>
#include "../../engine/GameEngine.hpp"
#include "../../engine/data/RenderTargetHandle.hpp"
#include "../../engine/data/components/Transform.hpp"
#include "../../engine/systems/assets/AssetManager.hpp"
#include "../../engine/systems/assets/BuiltinAssetNames.hpp"
#include "../../engine/systems/input/RaycastPickingSystem.hpp"
#include "../../engine/systems/ui/UserInterface.hpp"
#include "controller/EditorPickingSystem.hpp"
#include "controller/EditorUIController.hpp"
#include "controller/OrbitCameraController.hpp"
#include "controller/RenderingController.hpp"
#include "state/EditorState.hpp"
#include "view/imgui_widgets/containers/HierarchyPanel.hpp"
#include "view/imgui_widgets/containers/InspectorPanel.hpp"
#include "view/imgui_widgets/containers/RenderTargetPreview.hpp"

class GameWindow;
class EditorRenderSystem;
class VulkanEditorRenderer;
class RendererSettings;

class EditorApp {
public:
    EditorApp(GameWindow& window,
              GameEngine& engine,
              EditorState& editorState,
              EditorRenderSystem& editorRenderSystem,
              VulkanEditorRenderer& editorRenderer,
              RendererSettings& rendererSettings,
              AssetManager& assetManager,
              UserInterface& userInterface) :
        window_(window),
        engine_(engine),
        editorState_(editorState),
        assetManager_(assetManager),
        userInterface_(userInterface),
        renderingController_(editorState_, editorRenderer, editorRenderSystem, rendererSettings, assetManager),
        uiController_(engine_,
                      userInterface,
                      editorState_,
                      [this](const std::string& objectId, bool show) {
                          if (show)
                              enableObjectAABB(objectId);
                          else
                              disableObjectAABB(objectId);
                      }),
        cameraController_(engine_),
        pickingSystem_(assetManager) {
        setupViewport();
        setupScene();
        setupRenderTargetPreview();
    }

    void update(double deltaTime) {
        handleInput(deltaTime);
        updateCamera(deltaTime);
        updateRenderTarget();
    }

private:
    GameWindow& window_;
    GameEngine& engine_;
    EditorState& editorState_;
    AssetManager& assetManager_;
    UserInterface& userInterface_;
    RenderingController renderingController_;
    EditorUIController uiController_;
    OrbitCameraController cameraController_;
    EditorPickingSystem pickingSystem_;

    bool wasLeftDown_ = false;
    RenderTargetHandle renderTarget_;
    Camera previewCamera_;
    Transform previewCameraTransform_;

    std::unordered_set<std::string> aabbEnabled_;
    bool bvhEnabled_ = false;
    GizmoType gizmoMode_ = GizmoType::Translation; // T = Translation, R = Rotation, Y = Scale

    // Translation drag state
    struct TranslationDrag {
        std::string objectId;
        GizmoAxis axis;
        glm::vec3 axisDir; // world-space unit vector for the constrained axis
        glm::vec3 dragPlaneNormal; // normal of the constraint plane
        glm::vec3 dragOrigin; // object position at drag start
        glm::vec3 hitPointOnAxis; // first hit point projected onto axis
    };
    std::optional<TranslationDrag> activeDrag_;

    // Rotation drag state
    struct RotationDrag {
        std::string objectId;
        GizmoAxis axis;
        glm::vec3 axisDir; // world-space rotation axis
        glm::vec3 origin; // object position (center of rotation)
        glm::quat initialRotation; // object quaternion at drag start
        glm::vec3 initialHitDir; // normalized direction from origin to first hit in ring plane
    };
    std::optional<RotationDrag> activeRotationDrag_;

    // Scale drag state
    struct ScaleDrag {
        std::string objectId;
        GizmoAxis axis;
        glm::vec3 axisDir; // world-space drag direction (camera right for uniform)
        glm::vec3 origin; // object position
        glm::vec3 initialScale; // object scale at drag start
        glm::vec3 dragPlaneNormal;
        float initialT; // projected distance at drag start (must be non-zero)
    };
    std::optional<ScaleDrag> activeScaleDrag_;

    static SceneViewport computeSceneViewport(int w, int h) {
        int left = static_cast<int>(static_cast<float>(w) * HierarchyPanel::PANEL_WIDTH_RATIO);
        int right = static_cast<int>(static_cast<float>(w) * (1.0f - InspectorPanel::PANEL_WIDTH_RATIO));
        return {left, 0, right - left, h};
    }

    void setupViewport() {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport(computeSceneViewport(w, h));
        window_.onWindowResize([this](int newW, int newH, int, int) {
            SceneViewport sv = computeSceneViewport(newW, newH);
            window_.setSceneViewport(sv);
            if (sv.width > 0 && sv.height > 0)
                cameraController_.setAspect(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        });
    }

    void setupScene() {
        SceneViewport sv = window_.getSceneViewport();
        if (sv.width > 0 && sv.height > 0)
            cameraController_.setAspect(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        engine_.createCube({});
        renderingController_.enableWorldGrid();
    }

    void setupRenderTargetPreview() {
        renderTarget_ = renderingController_.createRenderTarget(512, 512);
        previewCamera_.aspect = 1.0f;
        previewCamera_.fov = 60.0f;
        previewCameraTransform_.position = {0.0f, 15.0f, 0.0f};
        previewCameraTransform_.scale = {1.0f, 1.0f, 1.0f};
        previewCameraTransform_.rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
        userInterface_.emplace<RenderTargetPreview>(engine_, renderingController_, renderTarget_);
    }

    // Returns the world position where a ray intersects the drag plane, or nullopt if parallel.
    static std::optional<glm::vec3>
    rayPlaneIntersect(const Ray& ray, const glm::vec3& planePoint, const glm::vec3& planeNormal) {
        float denom = glm::dot(ray.direction, planeNormal);
        if (std::abs(denom) < 1e-5f) return std::nullopt;
        float t = glm::dot(planePoint - ray.origin, planeNormal) / denom;
        if (t < 0.0f) return std::nullopt;
        return ray.origin + ray.direction * t;
    }

    // Projects a world point onto the constrained axis, returning the scalar distance from origin.
    static float projectOnAxis(const glm::vec3& point, const glm::vec3& axisOrigin, const glm::vec3& axisDir) {
        return glm::dot(point - axisOrigin, axisDir);
    }

    Ray buildMouseRay(double mouseX, double mouseY) const {
        auto sv = window_.getSceneViewport();
        float ndcX = (static_cast<float>(mouseX) - static_cast<float>(sv.x)) / static_cast<float>(sv.width);
        float ndcY = (static_cast<float>(mouseY) - static_cast<float>(sv.y)) / static_cast<float>(sv.height);
        return RaycastPickingSystem::buildRay(
                {ndcX, ndcY}, cameraController_.getCamera(), cameraController_.getTransform());
    }

    void handleInput(double deltaTime) {
        auto [mouseX, mouseY] = engine_.getMousePosition();

        bool ctrlHeld = engine_.isKeyDown(GLFW_KEY_LEFT_CONTROL) || engine_.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
        bool shiftHeld = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT) || engine_.isKeyDown(GLFW_KEY_RIGHT_SHIFT);

        if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_S) && uiController_.scenePath)
            uiController_.saveScene(*uiController_.scenePath);
        if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_Z)) {
            if (shiftHeld)
                uiController_.mutations.undoHistory().redo();
            else
                uiController_.mutations.undoHistory().undo();
        }

        if (engine_.isKeyPressed(GLFW_KEY_G)) renderingController_.toggleWorldGrid();
        if (engine_.isKeyPressed(GLFW_KEY_L)) renderingController_.toggleLighting();
        if (engine_.isKeyPressed(GLFW_KEY_B)) bvhEnabled_ = !bvhEnabled_;
        if (engine_.isKeyPressed(GLFW_KEY_T)) gizmoMode_ = GizmoType::Translation;
        if (engine_.isKeyPressed(GLFW_KEY_R)) gizmoMode_ = GizmoType::Rotation;
        if (engine_.isKeyPressed(GLFW_KEY_Y)) gizmoMode_ = GizmoType::Scale;

        bool leftDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

        if (activeDrag_) {
            if (leftDown) {
                updateTranslationDrag(mouseX, mouseY);
            } else {
                commitTranslationDrag();
            }
        } else if (activeRotationDrag_) {
            if (leftDown) {
                updateRotationDrag(mouseX, mouseY);
            } else {
                commitRotationDrag();
            }
        } else if (activeScaleDrag_) {
            if (leftDown) {
                updateScaleDrag(mouseX, mouseY);
            } else {
                commitScaleDrag();
            }
        } else if (leftDown && !wasLeftDown_ && !ImGui::GetIO().WantCaptureMouse) {
            auto sv = window_.getSceneViewport();
            auto result = pickingSystem_.pick(static_cast<uint32_t>(mouseX),
                                              static_cast<uint32_t>(mouseY),
                                              static_cast<uint32_t>(sv.x),
                                              static_cast<uint32_t>(sv.y),
                                              static_cast<uint32_t>(sv.width),
                                              static_cast<uint32_t>(sv.height),
                                              cameraController_.getCamera(),
                                              cameraController_.getTransform(),
                                              editorState_.getScene());
            if (result) {
                std::visit([&](const auto& hit) { onPickResult(hit, mouseX, mouseY); }, *result);
            } else {
                editorState_.setSelectedObject({});
            }
        }

        wasLeftDown_ = leftDown;
        drawGizmos();
    }

    void onPickResult(const SceneObjectHit& hit, double /*mouseX*/, double /*mouseY*/) {
        auto current = editorState_.getSelectedObject();
        if (current == hit.objectId)
            editorState_.setSelectedObject({});
        else
            editorState_.setSelectedObject(hit.objectId);
    }

    void onPickResult(const GizmoHit& hit, double mouseX, double mouseY) {
        if (hit.type == GizmoType::Translation) {
            beginTranslationDrag(hit.axis, mouseX, mouseY);
        } else if (hit.type == GizmoType::Rotation) {
            beginRotationDrag(hit.axis, mouseX, mouseY);
        } else if (hit.type == GizmoType::Scale) {
            beginScaleDrag(hit.axis, mouseX, mouseY);
        }
    }

    void beginTranslationDrag(GizmoAxis axis, double mouseX, double mouseY) {
        auto selectedId = editorState_.getSelectedObject();
        if (!selectedId) return;

        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(*selectedId);
        if (!obj.has<Transform>()) return;

        glm::vec3 axisDir = axisToDir(axis);
        glm::vec3 origin = obj.get<Transform>().position;

        // Choose a drag plane whose normal is perpendicular to the axis and faces the camera as much as possible
        glm::vec3 camDir = glm::normalize(cameraController_.getTransform().position - origin);
        glm::vec3 planeNormal = glm::normalize(camDir - glm::dot(camDir, axisDir) * axisDir);
        if (glm::length(planeNormal) < 1e-5f) planeNormal = cameraController_.getTransform().getUp();

        Ray ray = buildMouseRay(mouseX, mouseY);
        auto hitPoint = rayPlaneIntersect(ray, origin, planeNormal);
        if (!hitPoint) return;

        // Begin edit for undo
        uiController_.mutations.beginEdit(obj.get<Transform>());

        activeDrag_ = TranslationDrag{
                *selectedId,
                axis,
                axisDir,
                planeNormal,
                origin,
                *hitPoint // first hit point on the drag plane
        };
    }

    void updateTranslationDrag(double mouseX, double mouseY) {
        if (!activeDrag_) return;
        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(activeDrag_->objectId);
        if (!obj.has<Transform>()) return;

        Ray ray = buildMouseRay(mouseX, mouseY);
        auto hitPoint = rayPlaneIntersect(ray, activeDrag_->dragOrigin, activeDrag_->dragPlaneNormal);
        if (!hitPoint) return;

        // Compute how far along the axis the mouse has moved from the initial hit point
        float startT = projectOnAxis(activeDrag_->hitPointOnAxis, activeDrag_->dragOrigin, activeDrag_->axisDir);
        float currentT = projectOnAxis(*hitPoint, activeDrag_->dragOrigin, activeDrag_->axisDir);
        float delta = currentT - startT;

        obj.get<Transform>().position = activeDrag_->dragOrigin + activeDrag_->axisDir * delta;
    }

    void commitTranslationDrag() {
        if (!activeDrag_) return;
        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(activeDrag_->objectId);
        if (obj.has<Transform>()) uiController_.mutations.commitEdit(obj.get<Transform>());
        activeDrag_.reset();
    }

    void beginRotationDrag(GizmoAxis axis, double mouseX, double mouseY) {
        auto selectedId = editorState_.getSelectedObject();
        if (!selectedId) return;

        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(*selectedId);
        if (!obj.has<Transform>()) return;

        glm::vec3 axisDir = axisToDir(axis);
        glm::vec3 origin = obj.get<Transform>().position;

        // The ring lives in the plane whose normal is the rotation axis
        Ray ray = buildMouseRay(mouseX, mouseY);
        auto hitPoint = rayPlaneIntersect(ray, origin, axisDir);
        if (!hitPoint) return;

        glm::vec3 hitDir = *hitPoint - origin;
        if (glm::length(hitDir) < 1e-5f) return;

        hitDir = glm::normalize(hitDir);

        uiController_.mutations.beginEdit(obj.get<Transform>());
        activeRotationDrag_ = RotationDrag{*selectedId, axis, axisDir, origin, obj.get<Transform>().rotation, hitDir};
    }

    void updateRotationDrag(double mouseX, double mouseY) {
        if (!activeRotationDrag_) return;
        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(activeRotationDrag_->objectId);
        if (!obj.has<Transform>()) return;

        Ray ray = buildMouseRay(mouseX, mouseY);
        auto hitPoint = rayPlaneIntersect(ray, activeRotationDrag_->origin, activeRotationDrag_->axisDir);
        if (!hitPoint) return;

        glm::vec3 hitDir = *hitPoint - activeRotationDrag_->origin;
        if (glm::length(hitDir) < 1e-5f) return;
        hitDir = glm::normalize(hitDir);

        // Signed angle from initial hit direction to current, around the rotation axis
        float cosA = glm::clamp(glm::dot(activeRotationDrag_->initialHitDir, hitDir), -1.0f, 1.0f);
        float angle = glm::acos(cosA);
        if (glm::dot(glm::cross(activeRotationDrag_->initialHitDir, hitDir), activeRotationDrag_->axisDir) < 0.0f)
            angle = -angle;

        obj.get<Transform>().rotation =
                glm::angleAxis(angle, activeRotationDrag_->axisDir) * activeRotationDrag_->initialRotation;
    }

    void commitRotationDrag() {
        if (!activeRotationDrag_) return;
        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(activeRotationDrag_->objectId);
        if (obj.has<Transform>()) uiController_.mutations.commitEdit(obj.get<Transform>());
        activeRotationDrag_.reset();
    }

    void beginScaleDrag(GizmoAxis axis, double mouseX, double mouseY) {
        auto selectedId = editorState_.getSelectedObject();
        if (!selectedId) return;
        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(*selectedId);
        if (!obj.has<Transform>()) return;

        glm::vec3 origin = obj.get<Transform>().position;

        // For uniform scale use the camera's right vector as drag axis;
        // for single-axis use the world axis with a camera-facing plane.
        glm::vec3 axisDir;
        glm::vec3 planeNormal;
        if (axis == GizmoAxis::All) {
            axisDir = glm::normalize(cameraController_.getTransform().getRight());
            planeNormal = glm::normalize(cameraController_.getTransform().position - origin);
        } else {
            axisDir = axisToDir(axis);
            glm::vec3 camDir = glm::normalize(cameraController_.getTransform().position - origin);
            planeNormal = glm::normalize(camDir - glm::dot(camDir, axisDir) * axisDir);
            if (glm::length(planeNormal) < 1e-5f) planeNormal = cameraController_.getTransform().getUp();
        }

        Ray ray = buildMouseRay(mouseX, mouseY);
        auto hitPoint = rayPlaneIntersect(ray, origin, planeNormal);
        if (!hitPoint) return;

        float initialT = glm::dot(*hitPoint - origin, axisDir);
        if (std::abs(initialT) < 1e-5f) return;

        uiController_.mutations.beginEdit(obj.get<Transform>());
        activeScaleDrag_ =
                ScaleDrag{*selectedId, axis, axisDir, origin, obj.get<Transform>().scale, planeNormal, initialT};
    }

    void updateScaleDrag(double mouseX, double mouseY) {
        if (!activeScaleDrag_) return;
        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(activeScaleDrag_->objectId);
        if (!obj.has<Transform>()) return;

        Ray ray = buildMouseRay(mouseX, mouseY);
        auto hitPoint = rayPlaneIntersect(ray, activeScaleDrag_->origin, activeScaleDrag_->dragPlaneNormal);
        if (!hitPoint) return;

        float currentT = glm::dot(*hitPoint - activeScaleDrag_->origin, activeScaleDrag_->axisDir);
        float factor = glm::max(currentT / activeScaleDrag_->initialT, 0.01f);

        if (activeScaleDrag_->axis == GizmoAxis::All) {
            obj.get<Transform>().scale = activeScaleDrag_->initialScale * factor;
        } else {
            glm::vec3 newScale = activeScaleDrag_->initialScale;
            switch (activeScaleDrag_->axis) {
                case GizmoAxis::X:
                    newScale.x = activeScaleDrag_->initialScale.x * factor;
                    break;
                case GizmoAxis::Y:
                    newScale.y = activeScaleDrag_->initialScale.y * factor;
                    break;
                case GizmoAxis::Z:
                    newScale.z = activeScaleDrag_->initialScale.z * factor;
                    break;
                default:
                    break;
            }
            obj.get<Transform>().scale = newScale;
        }
    }

    void commitScaleDrag() {
        if (!activeScaleDrag_) return;
        auto& scene = editorState_.getScene();
        auto& obj = scene.getObject(activeScaleDrag_->objectId);
        if (obj.has<Transform>()) uiController_.mutations.commitEdit(obj.get<Transform>());
        activeScaleDrag_.reset();
    }

    static glm::vec3 axisToDir(GizmoAxis axis) {
        switch (axis) {
            case GizmoAxis::X:
                return {1, 0, 0};
            case GizmoAxis::Y:
                return {0, 1, 0};
            case GizmoAxis::Z:
                return {0, 0, 1};
            case GizmoAxis::All:
                return {1, 1, 1};
        }
        return {1, 0, 0};
    }

    void drawGizmos() {
        pickingSystem_.clearHandles(); // re-registered below each frame

        if (auto selectedId = editorState_.getSelectedObject()) {
            renderingController_.drawObjectOutline(*selectedId);

            // Draw and register handles for the active gizmo mode
            std::vector<GizmoHandle> handles;
            if (gizmoMode_ == GizmoType::Translation) {
                handles = renderingController_.drawTranslationHandles(
                        *selectedId, cameraController_.getCamera(), cameraController_.getTransform());
            } else if (gizmoMode_ == GizmoType::Rotation) {
                handles = renderingController_.drawRotationHandles(
                        *selectedId, cameraController_.getCamera(), cameraController_.getTransform());
            } else if (gizmoMode_ == GizmoType::Scale) {
                handles = renderingController_.drawScaleHandles(
                        *selectedId, cameraController_.getCamera(), cameraController_.getTransform());
            }
            for (const auto& h: handles)
                pickingSystem_.registerHandle(h);
        }

        for (const auto& objectId: aabbEnabled_)
            renderingController_.drawGizmoObjectAABB(objectId, {0.0f, 1.0f, 0.0f});

        if (bvhEnabled_) renderingController_.drawGizmoBVH({1.0f, 1.0f, 0.0f});
    }

    void enableObjectAABB(const std::string& objectId) { aabbEnabled_.insert(objectId); }
    void disableObjectAABB(const std::string& objectId) { aabbEnabled_.erase(objectId); }
    void toggleBVH() { bvhEnabled_ = !bvhEnabled_; }

    void updateCamera(double deltaTime) {
        cameraController_.update(deltaTime);
        renderingController_.setActiveCamera(cameraController_.getCamera(), cameraController_.getTransform());
    }

    void updateRenderTarget() {
        if (renderTarget_.isValid())
            renderingController_.renderToTarget(renderTarget_, previewCamera_, previewCameraTransform_);
    }
};
