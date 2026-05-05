#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include "../../../engine/data/RenderTargetHandle.hpp"
#include "../../../engine/data/components/Camera.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "../../../engine/systems/rendering/vulkan/core/utils/structs.hpp"
#include "../../../engine/systems/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../engine/systems/scene/SceneManager.hpp"

/*
    EditorState

    Purpose:
    --------------------------------------------------
    Central model containing all editor-specific state.
    Serves as the single source of truth for editor UI and controllers.

    Contents:
    --------------------------------------------------
    - SceneManager reference (game objects being edited)
    - Gizmo drawing queue (lines to visualize)
    - Outline drawing queue (objects to outline)
    - Active editor camera and transform
    - Currently selected object for editing
    - Rendering settings (grid, lighting)
    - Camera and render target state
*/
class EditorState {
public:
    explicit EditorState(SceneManager& scene) : sceneManager_(scene) {}

    // --------------------------------------------------------
    // SceneManager
    // --------------------------------------------------------
    SceneManager& getScene() { return sceneManager_; }
    const SceneManager& getScene() const { return sceneManager_; }

    // --------------------------------------------------------
    // Selection
    // --------------------------------------------------------
    void setSelectedObject(const std::optional<std::string>& objectId) { selectedObjectId_ = objectId; }
    const std::optional<std::string>& getSelectedObject() const { return selectedObjectId_; }
    std::optional<std::string>& getSelectedObjectRef() { return selectedObjectId_; }
    bool hasSelectedObject() const { return selectedObjectId_.has_value(); }

    // --------------------------------------------------------
    // Rendering Flags
    // --------------------------------------------------------
    bool isGridEnabled() const { return gridEnabled_; }
    void setGridEnabled(bool enabled) { gridEnabled_ = enabled; }
    void toggleGrid() { gridEnabled_ = !gridEnabled_; }
    float getGridScale() const { return gridScale_; }
    void setGridScale(float scale) { gridScale_ = scale; }

    bool isLightingEnabled() const { return lightingEnabled_; }
    void setLightingEnabled(bool enabled) { lightingEnabled_ = enabled; }
    void toggleLighting() { lightingEnabled_ = !lightingEnabled_; }

    // --------------------------------------------------------
    // Gizmo State
    // --------------------------------------------------------
    std::vector<VulkanGizmoPass::GizmoVertex>& getGizmoLines() { return gizmoLines_; }
    const std::vector<VulkanGizmoPass::GizmoVertex>& getGizmoLines() const { return gizmoLines_; }
    void clearGizmoLines() { gizmoLines_.clear(); }

    // --------------------------------------------------------
    // Outline State
    // --------------------------------------------------------
    std::vector<DrawCall>& getOutlineQueue() { return outlineQueue_; }
    const std::vector<DrawCall>& getOutlineQueue() const { return outlineQueue_; }
    void clearOutlineQueue() { outlineQueue_.clear(); }

    // --------------------------------------------------------
    // Bounding Volume Visualization
    // --------------------------------------------------------
    std::unordered_set<std::string>& getAABBEnabledObjects() { return aabbEnabled_; }
    const std::unordered_set<std::string>& getAABBEnabledObjects() const { return aabbEnabled_; }
    void enableAABB(const std::string& objectId) { aabbEnabled_.insert(objectId); }
    void disableAABB(const std::string& objectId) { aabbEnabled_.erase(objectId); }

    std::unordered_set<std::string>& getBoundingSphereEnabledObjects() { return boundingSpheresEnabled_; }
    const std::unordered_set<std::string>& getBoundingSphereEnabledObjects() const { return boundingSpheresEnabled_; }
    void enableBoundingSphere(const std::string& objectId) { boundingSpheresEnabled_.insert(objectId); }
    void disableBoundingSphere(const std::string& objectId) { boundingSpheresEnabled_.erase(objectId); }

    bool isBVHEnabled() const { return bvhEnabled_; }
    void setBVHEnabled(bool enabled) { bvhEnabled_ = enabled; }
    void toggleBVH() { bvhEnabled_ = !bvhEnabled_; }


    // --------------------------------------------------------
    // Active Camera
    // --------------------------------------------------------
    void setActiveCamera(const Camera& camera, const Transform& transform) {
        activeCamera_ = &camera;
        activeCameraTransform_ = &transform;
    }

    Camera getActiveCamera() const {
        if (activeCamera_ != nullptr) return *activeCamera_;
        return Camera();
    }

    Transform getActiveCameraTransform() const {
        if (activeCameraTransform_ != nullptr) return *activeCameraTransform_;
        return Transform();
    }

    // --------------------------------------------------------
    // Render Targets
    // --------------------------------------------------------
    void setRenderTargetImGuiId(RenderTargetHandle handle, VkDescriptorSet id) {
        renderTargetImGuiIds_[handle.index] = id;
    }

    VkDescriptorSet getRenderTargetImGuiId(RenderTargetHandle handle) const {
        auto it = renderTargetImGuiIds_.find(handle.index);
        return it != renderTargetImGuiIds_.end() ? it->second : VK_NULL_HANDLE;
    }

private:
    SceneManager& sceneManager_;
    std::optional<std::string> selectedObjectId_;
    std::vector<VulkanGizmoPass::GizmoVertex> gizmoLines_;
    std::vector<DrawCall> outlineQueue_;
    std::unordered_set<std::string> aabbEnabled_;
    std::unordered_set<std::string> boundingSpheresEnabled_;
    const Camera* activeCamera_ = nullptr;
    const Transform* activeCameraTransform_ = nullptr;
    bool gridEnabled_ = true;
    bool lightingEnabled_ = true;
    bool bvhEnabled_ = false;
    float gridScale_ = 1.0f;
    std::unordered_map<uint32_t, VkDescriptorSet> renderTargetImGuiIds_;
};
