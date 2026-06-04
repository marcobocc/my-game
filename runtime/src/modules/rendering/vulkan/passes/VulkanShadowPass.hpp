#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <span>
#include <vector>
#include <volk.h>
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/structs.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Transform.hpp"

class VulkanShadowPass {
public:
    static constexpr VkFormat SHADOW_DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
    static constexpr uint32_t SHADOW_MAP_SIZE = 1024;
    static constexpr uint32_t MAX_SHADOW_LIGHTS = 4;
    static constexpr float DEPTH_BIAS_CONSTANT = 1.0f;
    static constexpr float DEPTH_BIAS_SLOPE = 1.0f;

    VulkanShadowPass(VulkanResourcesManager& resourcesManager, AssetLoader& assetLoader) :
        resourcesManager_(resourcesManager),
        assetLoader_(assetLoader) {
        lightSpaceMatrices_.fill(glm::mat4(1.0f));
        initPipeline();
    }

    // Returns the light-space matrix computed for slot i in the last record() call.
    const glm::mat4& lightSpaceMatrix(uint32_t i) const { return lightSpaceMatrices_[i]; }

    void record(VkCommandBuffer cmd,
                VkImageView shadowDepthView,
                const std::vector<DrawCall>& drawQueue,
                const std::vector<std::pair<Light, Transform>>& lightsWithTransforms,
                const Camera& camera,
                const Transform& cameraTransform,
                uint32_t lightIndex) {
        if (!pipeline_ || lightIndex >= lightsWithTransforms.size()) return;

        lightSpaceMatrices_[lightIndex] =
                computeLightSpaceMatrix({lightsWithTransforms[lightIndex]}, camera, cameraTransform);

        VkClearValue clearDepth{};
        clearDepth.depthStencil = {1.0f, 0};

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = shadowDepthView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue = clearDepth;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 0;
        renderingInfo.pDepthAttachment = &depthAttachment;
        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
        viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);

        // Pipeline enables dynamic depth bias; values must be set or they are undefined.
        // Slope-scaled bias offsets each surface by its depth gradient w.r.t. the light,
        // which is what prevents flat faces from self-shadowing.
        vkCmdSetDepthBias(cmd, DEPTH_BIAS_CONSTANT, 0.0f, DEPTH_BIAS_SLOPE);

        for (const auto& drawCall: drawQueue)
            renderEntity(cmd, drawCall, lightIndex);

        vkCmdEndRendering(cmd);
    }

private:
    struct PushConstants {
        glm::mat4 lightSpaceMatrix;
        glm::mat4 model;
    };

    void initPipeline() {
        const Shader* shader = assetLoader_.get<Shader>(SHADOW_SHADER);
        if (!shader) return;
        // Depth-only pass: no color attachments.
        pipeline_ = &resourcesManager_.getPipeline(*shader, std::span<const VkFormat>{}, SHADOW_DEPTH_FORMAT, true);
    }

    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall, uint32_t lightIndex) const {
        const Mesh* mesh = assetLoader_.get<Mesh>(drawCall.renderer.meshName);
        if (!mesh) return;

        PushConstants pc{};
        pc.lightSpaceMatrix = lightSpaceMatrices_[lightIndex];
        pc.model = drawCall.transform.getModelMatrix();
        vkCmdPushConstants(cmd,
                           pipeline_->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(PushConstants),
                           &pc);

        const auto& meshBuffers = resourcesManager_.getMesh(*mesh);
        std::array<VkDeviceSize, 1> offsets{};
        vkCmdBindVertexBuffers(cmd, 0, 1, &meshBuffers.vertexBuffer.buffer, offsets.data());
        vkCmdBindIndexBuffer(cmd, meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        if (mesh->hasIndices())
            vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh->getIndices().size()), 1, 0, 0, 0);
        else
            vkCmdDraw(cmd, static_cast<uint32_t>(mesh->getVertexCount()), 1, 0, 0);
    }

    // Vulkan's depth attachment is [0,1], but GLM (no GLM_FORCE_DEPTH_ZERO_TO_ONE) emits NDC
    // z in [-1,1]. This matrix remaps z from [-1,1] to [0,1] so the shadow map stores usable
    // depth instead of clamping the near half. The lighting shader expects this convention.
    static glm::mat4 zCorrectMatrix() {
        glm::mat4 zCorrect(1.0f);
        zCorrect[2][2] = 0.5f;
        zCorrect[3][2] = 0.5f;
        return zCorrect;
    }

    static glm::mat4 computeLightSpaceMatrix(const std::vector<std::pair<Light, Transform>>& lights,
                                             const Camera& camera,
                                             const Transform& cameraTransform) {
        if (lights.empty()) return glm::mat4(1.0f);

        if (lights[0].first.type == LightType::SPOT)
            return computeSpotLightSpaceMatrix(lights[0].first, lights[0].second);

        const Transform& t = lights[0].second;
        const glm::vec3 lightDir = glm::normalize(t.getForward());
        const glm::vec3 up =
                (std::abs(glm::dot(lightDir, glm::vec3(0, 1, 0))) > 0.99f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

        // Compute the 8 corners of the camera frustum in world space.
        const float nearH = camera.nearPlane * std::tan(glm::radians(camera.fov * 0.5f));
        const float nearW = nearH * camera.aspect;

        float shadowDistance = 25.0f;

        const float farH = shadowDistance * std::tan(glm::radians(camera.fov * 0.5f));
        const float farW = farH * camera.aspect;

        const glm::vec3 camPos = cameraTransform.position;
        const glm::vec3 camFwd = cameraTransform.getForward();
        const glm::vec3 camRight = cameraTransform.getRight();
        const glm::vec3 camUp = cameraTransform.getUp();

        const glm::vec3 nc = camPos + camFwd * camera.nearPlane;
        const glm::vec3 fc = camPos + camFwd * shadowDistance;

        const glm::vec3 corners[8] = {
                nc + camUp * nearH - camRight * nearW,
                nc + camUp * nearH + camRight * nearW,
                nc - camUp * nearH - camRight * nearW,
                nc - camUp * nearH + camRight * nearW,
                fc + camUp * farH - camRight * farW,
                fc + camUp * farH + camRight * farW,
                fc - camUp * farH - camRight * farW,
                fc - camUp * farH + camRight * farW,
        };

        // Centre the light view on the frustum centre so the eye is never
        // inside or behind the frustum regardless of camera orientation.
        glm::vec3 frustumCenter(0.0f);
        for (const glm::vec3& c: corners)
            frustumCenter += c;
        frustumCenter /= 8.0f;

        // lightDir is the direction the light travels (Transform forward). The shadow camera must
        // look ALONG that direction, so place the eye upstream (frustumCenter - lightDir) looking
        // toward the centre. This matches the spotlight path, which looks along +forward.
        const glm::mat4 lightView = glm::lookAt(frustumCenter - lightDir, frustumCenter, up);

        // Find the AABB of the frustum corners in light space.
        float minX = std::numeric_limits<float>::max();
        float maxX = -std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxY = -std::numeric_limits<float>::max();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = -std::numeric_limits<float>::max();

        for (const glm::vec3& c: corners) {
            const glm::vec4 ls = lightView * glm::vec4(c, 1.0f);
            minX = std::min(minX, ls.x);
            maxX = std::max(maxX, ls.x);
            minY = std::min(minY, ls.y);
            maxY = std::max(maxY, ls.y);
            minZ = std::min(minZ, ls.z);
            maxZ = std::max(maxZ, ls.z);
        }

        // lightView (glm::lookAt, right-handed) looks down -z, so frustum corners have
        // negative view-space z. glm::ortho expects positive near/far *distances*, so convert:
        // the nearest corner (largest, least-negative z) gives the near distance, the farthest
        // (most-negative z) gives the far distance.
        float nearDist = -maxZ;
        float farDist = -minZ;

        // Pull the near plane toward the light so geometry just outside the camera frustum still
        // casts shadows. Keep this small: inflating the ortho depth range flattens the per-texel
        // depth slope, which makes the slope-scaled hardware depth bias ineffective and causes
        // whole-face self-shadowing.
        constexpr float zPull = 10.0f;
        nearDist -= zPull;

        glm::mat4 proj = glm::ortho(minX, maxX, minY, maxY, nearDist, farDist);
        proj[1][1] *= -1.0f;

        return zCorrectMatrix() * proj * lightView;
    }

    // Spotlights cast through a perspective frustum centred on the cone axis, from the light's
    // position. The FOV is the full outer cone angle (plus a small margin so the cone edge isn't
    // clipped), and the far plane is the light's range.
    static glm::mat4 computeSpotLightSpaceMatrix(const Light& light, const Transform& transform) {
        const glm::vec3 lightPos = transform.position;
        const glm::vec3 lightDir = glm::normalize(transform.getForward());
        const glm::vec3 up =
                (std::abs(glm::dot(lightDir, glm::vec3(0, 1, 0))) > 0.99f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

        const glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, up);

        constexpr float FOV_MARGIN = 1.1f;
        constexpr float NEAR_PLANE = 0.1f;
        const float fov = glm::radians(std::min(light.outerConeAngle * 2.0f * FOV_MARGIN, 179.0f));
        const float farPlane = std::max(light.range, NEAR_PLANE + 0.01f);

        glm::mat4 proj = glm::perspective(fov, 1.0f, NEAR_PLANE, farPlane);
        proj[1][1] *= -1.0f;

        return zCorrectMatrix() * proj * lightView;
    }

    VulkanResourcesManager& resourcesManager_;
    AssetLoader& assetLoader_;

    VulkanPipeline* pipeline_ = nullptr;
    std::array<glm::mat4, MAX_SHADOW_LIGHTS> lightSpaceMatrices_{};
};
