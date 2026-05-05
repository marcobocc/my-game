#pragma once
#include <string>
#include "../../../engine/data/RenderTargetHandle.hpp"
#include "../../../engine/data/components/Camera.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "../state/EditorState.hpp"

class VulkanEditorRenderer;
class EditorRenderSystem;
class AssetManager;
struct RendererSettings;

/*
    RenderingController

    Purpose:
    --------------------------------------------------
    Handles all rendering-related operations for the editor.

    Responsibilities:
    --------------------------------------------------
    - Grid and lighting toggle logic
    - Render target creation and management
    - Gizmo drawing (lines, AABBs, transforms, BVH)
    - Object outline management
    - Camera management for rendering
*/
class RenderingController {
public:
    RenderingController(EditorState& state,
                        VulkanEditorRenderer& renderer,
                        EditorRenderSystem& renderSystem,
                        RendererSettings& settings,
                        AssetManager& assetManager);

    // --------------------------------------------------------
    // Grid Control
    // --------------------------------------------------------
    void enableWorldGrid();
    void disableWorldGrid();
    void toggleWorldGrid();

    // --------------------------------------------------------
    // Lighting Control
    // --------------------------------------------------------
    void enableLighting();
    void disableLighting();
    void toggleLighting();

    // --------------------------------------------------------
    // Camera Management
    // --------------------------------------------------------
    void setActiveCamera(const Camera& camera, const Transform& cameraTransform);

    // --------------------------------------------------------
    // Render Targets
    // --------------------------------------------------------
    RenderTargetHandle createRenderTarget(uint32_t width, uint32_t height);
    void destroyRenderTarget(RenderTargetHandle handle);
    VkDescriptorSet getRenderTargetImGuiId(RenderTargetHandle handle) const;
    void renderToTarget(RenderTargetHandle handle, const Camera& camera, const Transform& cameraTransform);

    // --------------------------------------------------------
    // Object Outlining
    // --------------------------------------------------------
    void drawObjectOutline(const std::string& objectId);

private:
    EditorState& editorState_;
    VulkanEditorRenderer& renderer_;
    EditorRenderSystem& renderSystem_;
    RendererSettings& settings_;
    AssetManager& assetManager_;
};
