#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <volk.h>
#include "../../../core/components/Transform.hpp"
#include "../../assets/Mesh.hpp"
#include "../../components/Camera.hpp"
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/structs.hpp"
#include "core/assets/AssetLoader.hpp"
#include "core/assets/BuiltinAssetNames.hpp"

class VulkanSkyPass {
public:
    VulkanSkyPass(VulkanContext& context, AssetLoader& assetLoader, VulkanResourcesManager& resourcesManager) :
        assetLoader_(assetLoader),
        resourcesManager_(resourcesManager),
        skydome_(buildSphereMesh()) {
        initPipeline();
        meshBuffers_ = &resourcesManager_.getMesh(skydome_);
        indexCount_ = static_cast<uint32_t>(skydome_.getIndices().size());
    }

    ~VulkanSkyPass() = default;

    void record(VkCommandBuffer cmd,
                VkImageView colorView,
                VkImageView depthView,
                VkExtent2D extent,
                const Camera& camera,
                const Transform& cameraTransform,
                int fbX,
                int fbY,
                int fbW,
                int fbH) const {
        if (!pipeline_ || !meshBuffers_) return;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depthView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, extent};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport{static_cast<float>(fbX),
                            static_cast<float>(fbY),
                            static_cast<float>(fbW),
                            static_cast<float>(fbH),
                            0.0f,
                            1.0f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{fbX, fbY}, {static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH)}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        struct SkyPushConstants {
            glm::mat4 viewProj;
            glm::vec3 cameraPos;
            float pad;
        } pc{};
        pc.viewProj = camera.getProjectionMatrix() * cameraTransform.getViewMatrix();
        pc.cameraPos = cameraTransform.position;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &meshBuffers_->vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(cmd, meshBuffers_->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdPushConstants(cmd,
                           pipeline_->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(SkyPushConstants),
                           &pc);

        vkCmdDrawIndexed(cmd, indexCount_, 1, 0, 0, 0);
        vkCmdEndRendering(cmd);
    }

private:
    static Mesh buildSphereMesh() {
        constexpr uint32_t res = 32;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;
        std::vector<uint32_t> indices;

        for (uint32_t lat = 0; lat <= res; ++lat) {
            float theta = glm::pi<float>() * static_cast<float>(lat) / static_cast<float>(res);
            float sinTheta = glm::sin(theta);
            float cosTheta = glm::cos(theta);
            for (uint32_t lon = 0; lon <= res; ++lon) {
                float phi = 2.0f * glm::pi<float>() * static_cast<float>(lon) / static_cast<float>(res);
                glm::vec3 normal(sinTheta * glm::cos(phi), cosTheta, sinTheta * glm::sin(phi));
                positions.push_back(normal);
                normals.push_back(normal);
                uvs.push_back({static_cast<float>(lon) / static_cast<float>(res),
                               static_cast<float>(lat) / static_cast<float>(res)});
            }
        }
        for (uint32_t lat = 0; lat < res; ++lat) {
            for (uint32_t lon = 0; lon < res; ++lon) {
                uint32_t a = lat * (res + 1) + lon;
                uint32_t b = a + res + 1;
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);
                indices.push_back(a + 1);
                indices.push_back(b);
                indices.push_back(b + 1);
            }
        }
        std::vector<glm::vec3> colors(positions.size(), glm::vec3(1.0f));
        return Mesh("_SKYDOME",
                    std::move(positions),
                    std::move(uvs),
                    std::move(colors),
                    std::move(indices),
                    std::move(normals));
    }

    void initPipeline() {
        const Shader* shader = assetLoader_.get<Shader>(SKYDOME_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(
                *shader, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT, /*cullFront=*/true);
    }

    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;

    Mesh skydome_;
    VulkanMeshBuffers* meshBuffers_ = nullptr;
    VulkanPipeline* pipeline_ = nullptr;
    uint32_t indexCount_ = 0;
};
