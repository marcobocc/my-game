#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <span>
#include <unordered_map>
#include <vector>
#include <volk.h>
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/buffers.hpp"
#include "../core/utils/descriptors.hpp"
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

    VulkanShadowPass(VulkanResourcesManager& resourcesManager, AssetLoader& assetLoader, const VulkanContext& context) :
        resourcesManager_(resourcesManager),
        assetLoader_(assetLoader),
        context_(context) {
        lightSpaceMatrices_.fill(glm::mat4(1.0f));
        initPipelines();
        createSkinningUBOLayout();
    }

    ~VulkanShadowPass() {
        for (auto& [id, entry]: skinningBuffers_)
            destroyBuffer(context_.device, entry.buffer);
        if (skinningUBOLayout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(context_.device, skinningUBOLayout_, nullptr);
    }

    // Returns the light-space matrix computed for slot i in the last record() call.
    // For point lights this is unused by the lighting pass (cube shadow sampling needs no matrix).
    const glm::mat4& lightSpaceMatrix(uint32_t i) const { return lightSpaceMatrices_[i]; }

    // Record shadow depth for one light slot into a 2D depth image (directional / spot).
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

        recordFace(cmd,
                   shadowDepthView,
                   drawQueue,
                   lightSpaceMatrices_[lightIndex],
                   /*isPointLight=*/false,
                   glm::vec3(0.0f),
                   0.0f);
    }

    // Record shadow depth for a point light into all 6 faces of a cubemap.
    // faceViews[0..5] must be per-face 2D views of the cubemap (face order: +X,-X,+Y,-Y,+Z,-Z).
    void recordPointLight(VkCommandBuffer cmd,
                          const std::array<VkImageView, 6>& faceViews,
                          const std::vector<DrawCall>& drawQueue,
                          const std::vector<std::pair<Light, Transform>>& lightsWithTransforms,
                          uint32_t lightIndex) {
        if (!pipelinePoint_ || lightIndex >= lightsWithTransforms.size()) return;

        const auto& [light, transform] = lightsWithTransforms[lightIndex];
        const glm::vec3 lightPos = transform.position;
        const float lightRange = std::max(light.range, 0.01f);

        // 6 face matrices: perspective (90° FOV, 1:1 aspect) looking along each cube axis.
        static const glm::vec3 faceDirections[6] = {
                {1, 0, 0},
                {-1, 0, 0},
                {0, 1, 0},
                {0, -1, 0},
                {0, 0, 1},
                {0, 0, -1},
        };
        static const glm::vec3 faceUps[6] = {
                {0, -1, 0},
                {0, -1, 0},
                {0, 0, 1},
                {0, 0, -1},
                {0, -1, 0},
                {0, -1, 0},
        };

        const glm::mat4 proj = [&] {
            glm::mat4 p = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, lightRange);
            p[1][1] *= -1.0f; // Vulkan Y flip
            return p;
        }();

        for (uint32_t face = 0; face < 6; ++face) {
            const glm::mat4 view = glm::lookAt(lightPos, lightPos + faceDirections[face], faceUps[face]);
            const glm::mat4 lightSpaceMat = proj * view;
            recordFace(cmd,
                       faceViews[face],
                       drawQueue,
                       lightSpaceMat,
                       /*isPointLight=*/true,
                       lightPos,
                       lightRange);
        }
    }

private:
    struct PushConstants {
        glm::mat4 lightSpaceMatrix;
        glm::mat4 model;
        // Point light fields (isPointLight == 1 activates linear depth write in frag shader).
        glm::vec3 lightPos;
        float lightRange;
        uint32_t isPointLight;
    };

    void recordFace(VkCommandBuffer cmd,
                    VkImageView depthView,
                    const std::vector<DrawCall>& drawQueue,
                    const glm::mat4& lightSpaceMat,
                    bool isPointLight,
                    const glm::vec3& lightPos,
                    float lightRange) {
        VulkanPipeline* activePipeline = isPointLight ? pipelinePoint_ : pipeline_;
        if (!activePipeline) return;

        VkClearValue clearDepth{};
        clearDepth.depthStencil = {1.0f, 0};

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depthView;
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

        for (const auto& drawCall: drawQueue)
            renderEntity(cmd, isPointLight, drawCall, lightSpaceMat, lightPos, lightRange);

        vkCmdEndRendering(cmd);
    }

    void createSkinningUBOLayout() {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = &binding;
        vkCreateDescriptorSetLayout(context_.device, &info, nullptr, &skinningUBOLayout_);
    }

    VkDescriptorSet getOrCreateSkinningDescriptorSet(const std::string& objectId) {
        auto it = skinningBuffers_.find(objectId);
        if (it != skinningBuffers_.end()) return it->second.descriptorSet;
        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * DRAW_CALL_MAX_BONES;
        SkinningEntry entry{};
        entry.buffer = createUniformBuffer(context_.device, context_.physicalDevice, uboSize);
        entry.descriptorSet = createUniformBufferDescriptorSet(
                context_.device, context_.descriptorPool, skinningUBOLayout_, entry.buffer.buffer, uboSize);
        auto [ins, _] = skinningBuffers_.emplace(objectId, std::move(entry));
        return ins->second.descriptorSet;
    }

    void updateSkinningBuffer(const std::string& objectId, const glm::mat4* bones) {
        auto it = skinningBuffers_.find(objectId);
        if (it == skinningBuffers_.end()) return;
        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * DRAW_CALL_MAX_BONES;
        updateBuffer(context_.device, it->second.buffer, bones, uboSize);
    }

    void uploadIdentitySkinning(const std::string& objectId) {
        static const auto identityBones = []() {
            std::array<glm::mat4, DRAW_CALL_MAX_BONES> arr{};
            arr.fill(glm::mat4(1.0f));
            return arr;
        }();
        updateSkinningBuffer(objectId, identityBones.data());
    }

    void initPipelines() {
        const Shader* shader = assetLoader_.get<Shader>(SHADOW_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(*shader, std::span<const VkFormat>{}, SHADOW_DEPTH_FORMAT, true);
        pipelinePoint_ = pipeline_;

        const Shader* skinnedShader = assetLoader_.get<Shader>(SHADOW_SKINNED_SHADER);
        if (skinnedShader) {
            skinnedPipeline_ = &resourcesManager_.getPipeline(
                    *skinnedShader, std::span<const VkFormat>{}, SHADOW_DEPTH_FORMAT, true);
            skinnedPipelinePoint_ = skinnedPipeline_;
        }
    }

    void renderEntity(VkCommandBuffer cmd,
                      bool isPointLight,
                      const DrawCall& drawCall,
                      const glm::mat4& lightSpaceMat,
                      const glm::vec3& lightPos,
                      float lightRange) {
        const Mesh* mesh = assetLoader_.get<Mesh>(drawCall.renderer.meshName);
        if (!mesh) return;

        const bool meshIsSkinned = mesh->isSkinned() && (skinnedPipeline_ != nullptr);
        VulkanPipeline* activePipeline = meshIsSkinned ? (isPointLight ? skinnedPipelinePoint_ : skinnedPipeline_)
                                                       : (isPointLight ? pipelinePoint_ : pipeline_);
        if (!activePipeline) return;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline->pipeline);

        if (meshIsSkinned) {
            VkDescriptorSet skinDS = getOrCreateSkinningDescriptorSet(drawCall.objectId);
            if (drawCall.skinningBones)
                updateSkinningBuffer(drawCall.objectId, drawCall.skinningBones);
            else
                uploadIdentitySkinning(drawCall.objectId);
            vkCmdBindDescriptorSets(
                    cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline->layout, 0, 1, &skinDS, 0, nullptr);
        }

        PushConstants pc{};
        pc.lightSpaceMatrix = lightSpaceMat;
        pc.model = drawCall.worldMatrix;
        pc.lightPos = lightPos;
        pc.lightRange = lightRange;
        pc.isPointLight = isPointLight ? 1u : 0u;
        vkCmdPushConstants(cmd,
                           activePipeline->layout,
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

        const float nearH = camera.nearPlane * std::tan(glm::radians(camera.fov * 0.5f));
        const float nearW = nearH * camera.aspect;

        float shadowDistance = 20.0f;

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

        glm::vec3 frustumCenter(0.0f);
        for (const glm::vec3& c: corners)
            frustumCenter += c;
        frustumCenter /= 8.0f;

        const glm::mat4 lightView = glm::lookAt(frustumCenter - lightDir, frustumCenter, up);

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

        float nearDist = -maxZ;
        float farDist = -minZ;

        constexpr float zPull = 10.0f;
        nearDist -= zPull;

        glm::mat4 proj = glm::ortho(minX, maxX, minY, maxY, nearDist, farDist);
        proj[1][1] *= -1.0f;

        return zCorrectMatrix() * proj * lightView;
    }

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

    struct SkinningEntry {
        VulkanBuffer buffer{};
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    VulkanResourcesManager& resourcesManager_;
    AssetLoader& assetLoader_;
    const VulkanContext& context_;

    VulkanPipeline* pipeline_ = nullptr;
    VulkanPipeline* pipelinePoint_ = nullptr;
    VulkanPipeline* skinnedPipeline_ = nullptr;
    VulkanPipeline* skinnedPipelinePoint_ = nullptr;
    VkDescriptorSetLayout skinningUBOLayout_ = VK_NULL_HANDLE;
    std::unordered_map<std::string, SkinningEntry> skinningBuffers_;
    std::array<glm::mat4, MAX_SHADOW_LIGHTS> lightSpaceMatrices_{};
};
