#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "../../../../runtime/src/core/assets/AssetCache.hpp"
#include "../../../../runtime/src/core/assets/AssetLoader.hpp"
#include "../../../../runtime/src/core/components/Transform.hpp"
#include "../../../../runtime/src/graphics/assets/Material.hpp"
#include "../../../../runtime/src/graphics/assets/Mesh.hpp"
#include "../../../../runtime/src/graphics/assets/Texture.hpp"
#include "../../../../runtime/src/graphics/components/Camera.hpp"
#include "../../../../runtime/src/graphics/components/Light.hpp"
#include "../../../../runtime/src/graphics/components/Renderer.hpp"
#include "../rendering/VulkanBackend.hpp"
#include "common_editing/details/AssetPrefabs.hpp"

/*
    MaterialPreviewRenderer

    Renders a lit mesh into a small off-screen render target and reads the pixels
    back to the CPU. Used by AssetThumbnailGenerator to build thumbnails for .mat
    assets (material on a sphere) and .mesh/.model assets (the mesh itself, camera
    framed to its bounds).

    Rendering piggybacks on the normal frame: a request queues an off-screen scene
    for the next renderFrame(); the result is read back on the following call
    (one-frame latency). Requests are serialized through a single shared render target.
*/
class MaterialPreviewRenderer {
    static constexpr uint32_t PREVIEW_SIZE = 256;
    static constexpr const char* SPHERE_MESH_NAME = "internal.editor/material_preview_sphere.mesh";

public:
    enum class Status { Pending, Ready, Failed };

    struct PreviewResult {
        Status status = Status::Pending;
        std::optional<Texture> texture;
    };

    MaterialPreviewRenderer(VulkanBackend& backend, AssetLoader& assetLoader, AssetCache& assetCache) :
        backend_(backend),
        assetLoader_(assetLoader),
        assetCache_(assetCache) {}

    // Renders the material applied to a unit sphere.
    PreviewResult requestPreview(const std::string& materialName) {
        if (!isReadbackTurn(materialName)) {
            if (!assetLoader_.get<Material>(materialName)) return {Status::Failed, std::nullopt};
        }
        return request(materialName, SPHERE_MESH_NAME, materialName, nullptr);
    }

    // Renders the given mesh with the given material, camera framed to the mesh bounds.
    // assetKey identifies the request (e.g. the .mesh or .model asset name).
    PreviewResult
    requestMeshPreview(const std::string& assetKey, const std::string& meshName, const std::string& materialName) {
        const Mesh* mesh = nullptr;
        if (!isReadbackTurn(assetKey)) {
            mesh = assetLoader_.get<Mesh>(meshName);
            if (!mesh) return {Status::Failed, std::nullopt};
            if (!assetLoader_.get<Material>(materialName)) return {Status::Failed, std::nullopt};
        }
        return request(assetKey, meshName, materialName, mesh);
    }

private:
    bool isReadbackTurn(const std::string& assetKey) const { return busy_ && assetKey == pendingAsset_; }

    // State machine: first call for an asset queues an off-screen render and returns
    // Pending; the next frame's call reads the pixels back and returns Ready. Only one
    // preview is in flight at a time — other assets stay Pending until their turn.
    PreviewResult request(const std::string& assetKey,
                          const std::string& meshName,
                          const std::string& materialName,
                          const Mesh* meshToFrame) {
        if (isReadbackTurn(assetKey)) {
            auto pixels = backend_.readbackRenderTarget(renderTarget_);
            busy_ = false;
            pendingAsset_.clear();
            if (pixels.empty()) return {Status::Failed, std::nullopt};
            return {Status::Ready,
                    Texture("preview://" + assetKey,
                            static_cast<int>(PREVIEW_SIZE),
                            static_cast<int>(PREVIEW_SIZE),
                            4,
                            std::move(pixels))};
        }
        if (busy_) return {Status::Pending, std::nullopt}; // another asset's turn

        ensureSetup();
        if (!renderTarget_.isValid()) return {Status::Failed, std::nullopt};

        if (meshToFrame) {
            BoundingSphere bounds = meshToFrame->getBoundingSphere();
            frameCamera(bounds.center, std::max(bounds.radius, 1e-3f));
        } else {
            frameCamera(glm::vec3(0.0f), 1.0f); // unit sphere
        }
        updateHeadlight();

        meshRenderer_.meshName = meshName;
        meshRenderer_.materialName = materialName;
        VulkanBackend::OffscreenSceneJob job;
        job.camera = camera_;
        job.cameraTransform = cameraTransform_;
        job.drawQueue.push_back(DrawCall{meshRenderer_, meshTransform_, meshTransform_.getModelMatrix()});
        job.lights = lights_;
        backend_.queueOffscreenScene(std::move(job));

        busy_ = true;
        pendingAsset_ = assetKey;
        return {Status::Pending, std::nullopt};
    }

    void ensureSetup() {
        if (setupDone_) return;
        setupDone_ = true;

        if (!assetCache_.contains(SPHERE_MESH_NAME))
            assetCache_.insert<Mesh>(std::make_unique<Mesh>(AssetPrefabs::sphere(48, SPHERE_MESH_NAME)));

        renderTarget_ = backend_.createRenderTarget(PREVIEW_SIZE, PREVIEW_SIZE);

        camera_.fov = 45.0f;
        camera_.aspect = 1.0f;
        camera_.renderTarget = renderTarget_;

        meshRenderer_.enabled = true;
    }

    // Places the camera slightly above and in front of the bounds so the object
    // fills most of the frame at 45° FOV, looking at the bounds center.
    void frameCamera(const glm::vec3& center, float radius) {
        float distance = radius / std::sin(glm::radians(camera_.fov) * 0.5f) * 1.1f;
        glm::vec3 offsetDir = glm::normalize(glm::vec3(0.0f, 0.35f, -2.9f));
        cameraTransform_.position = center + offsetDir * distance;
        glm::vec3 toCenter = glm::normalize(center - cameraTransform_.position);
        // Engine forward is +Z, but quatLookAt orients -Z along its direction — negate.
        cameraTransform_.rotation = glm::quatLookAt(-toCenter, glm::vec3(0.0f, 1.0f, 0.0f));

        camera_.nearPlane = std::max(distance - radius * 2.0f, distance * 0.01f);
        camera_.farPlane = distance + radius * 2.0f;
    }

    void updateHeadlight() {
        Light keyLight(LightType::DIRECTIONAL, 1.0f);
        keyLight.castShadows = false;
        // Headlight. The lighting shader computes L = -transform.forward, so sharing
        // the camera's rotation makes L point from the mesh back toward the camera.
        lights_.clear();
        lights_.emplace_back(keyLight, cameraTransform_);
    }

    VulkanBackend& backend_;
    AssetLoader& assetLoader_;
    AssetCache& assetCache_;

    bool setupDone_ = false;
    RenderTargetHandle renderTarget_{};
    Camera camera_{};
    Transform cameraTransform_{};
    // Stable storage: DrawCall holds references to these.
    Renderer meshRenderer_{};
    Transform meshTransform_{};
    std::vector<std::pair<Light, Transform>> lights_;

    bool busy_ = false;
    std::string pendingAsset_;
};
