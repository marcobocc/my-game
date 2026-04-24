#pragma once
#include <glm/glm.hpp>
#include <volk.h>
#include "assets/AssetManager.hpp"
#include "assets/types/shader/Shader.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/core/shaders.hpp"
#include "rendering/vulkan/core/structs.hpp"

class VulkanGridRenderer {
public:
    VulkanGridRenderer(const VulkanContext& context, AssetManager& assetManager) :
        context_(context),
        assetManager_(assetManager) {}

    ~VulkanGridRenderer() {
        if (pipeline_.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(context_.device, pipeline_.pipeline, nullptr);
        if (pipeline_.layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(context_.device, pipeline_.layout, nullptr);
    }

    void init(const std::string& shaderName) {
        const Shader* shader = assetManager_.get<Shader>(shaderName);
        if (!shader) return;

        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        pipeline_ = createGraphicsPipeline(
                context_.device, *shader, vertexInputState, {}, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT);
    }

    void drawGrid(VkCommandBuffer cmd, const Camera& camera, const Transform& cameraTransform) const {
        if (pipeline_.pipeline == VK_NULL_HANDLE) return;

        struct GridPushConstants {
            glm::mat4 invViewProj;
            glm::vec3 cameraPos;
            float _pad = 0.0f;
            glm::mat4 viewProj;
        } pc;

        glm::mat4 view = cameraTransform.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix();
        pc.viewProj = proj * view;
        pc.invViewProj = glm::inverse(pc.viewProj);
        pc.cameraPos = cameraTransform.position;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.pipeline);
        vkCmdPushConstants(cmd,
                           pipeline_.layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(GridPushConstants),
                           &pc);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }

private:
    const VulkanContext& context_;
    AssetManager& assetManager_;
    VulkanPipeline pipeline_{};
};
