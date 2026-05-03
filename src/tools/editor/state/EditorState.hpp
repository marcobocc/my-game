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
#include "../../../engine/systems/scene/Scene.hpp"

/*
    EditorState

    Purpose:
    --------------------------------------------------
    Central model containing all editor-specific state.
    Serves as the single source of truth for editor UI and controllers.

    Contents:
    --------------------------------------------------
    - Scene reference (game objects being edited)
    - Gizmo drawing queue (lines to visualize)
    - Outline drawing queue (objects to outline)
    - Active editor camera and transform
    - Currently selected object for editing
    - Rendering settings (grid, lighting)
    - Camera and render target state
*/
class EditorState {
public:
    explicit EditorState(Scene& scene) : scene_(scene) {}

    // --------------------------------------------------------
    // Scene
    // --------------------------------------------------------
    Scene& getScene() { return scene_; }
    const Scene& getScene() const { return scene_; }

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
    Scene& scene_;
    std::optional<std::string> selectedObjectId_;
    std::vector<VulkanGizmoPass::GizmoVertex> gizmoLines_;
    std::vector<DrawCall> outlineQueue_;
    const Camera* activeCamera_ = nullptr;
    const Transform* activeCameraTransform_ = nullptr;
    bool gridEnabled_ = true;
    bool lightingEnabled_ = true;
    std::unordered_map<uint32_t, VkDescriptorSet> renderTargetImGuiIds_;
};
