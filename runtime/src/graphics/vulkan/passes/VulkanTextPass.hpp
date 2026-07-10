#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <volk.h>
#include "../../../core/components/Transform.hpp"
#include "../../assets/Font.hpp"
#include "../../components/Camera.hpp"
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/buffers.hpp"
#include "../core/utils/descriptors.hpp"
#include "../core/utils/structs.hpp"
#include "core/GameWindow.hpp"
#include "core/assets/AssetLoader.hpp"
#include "core/assets/BuiltinAssetNames.hpp"
#include "graphics/GameRenderData.hpp"

class VulkanTextPass {
public:
    static constexpr uint32_t MAX_QUADS = 8192;

    struct TextVertex {
        glm::vec3 position;
        glm::vec2 uv;
        glm::vec4 color;
    };

    VulkanTextPass(const VulkanContext& context,
                   AssetLoader& assetLoader,
                   VulkanResourcesManager& resourcesManager,
                   uint32_t frameCount,
                   VkFormat colorFormat,
                   VkFormat depthFormat) :
        context_(context),
        assetLoader_(assetLoader),
        resourcesManager_(resourcesManager),
        colorFormat_(colorFormat),
        depthFormat_(depthFormat) {
        initPipeline();
        initVertexBuffers(frameCount);
    }

    ~VulkanTextPass() {
        for (auto& buf: vertexBuffers_)
            destroyBuffer(context_.device, buf);
    }

    void record(VkCommandBuffer cmd,
                VkImageView colorView,
                VkExtent2D extent,
                const Camera& camera,
                const Transform& cameraTransform,
                const GameWindow& window,
                const std::vector<TextDrawCall>& textQueue,
                VkImageView depthView) {
        if (pipeline_ == nullptr || textQueue.empty()) return;

        const uint32_t bufIdx = nextBufferIndex_++ % static_cast<uint32_t>(vertexBuffers_.size());

        // Build vertices grouped by font
        std::unordered_map<std::string, std::vector<TextVertex>> byFont;
        for (const auto& call: textQueue) {
            if (call.text.empty()) continue;
            Font* font = assetLoader_.get<Font>(call.fontName);
            if (!font) continue;
            buildQuads(*font, call, cameraTransform, byFont[call.fontName]);
        }

        if (byFont.empty()) return;

        // Collect all vertices into a single buffer for this frame
        std::vector<TextVertex> allVerts;
        allVerts.reserve(MAX_QUADS * 6);
        for (auto& [name, verts]: byFont)
            allVerts.insert(allVerts.end(), verts.begin(), verts.end());

        if (allVerts.empty()) return;
        const VkDeviceSize uploadSize = allVerts.size() * sizeof(TextVertex);
        updateBuffer(context_.device, vertexBuffers_[bufIdx], allVerts.data(), uploadSize);

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depthView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, extent};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(cmd, &renderingInfo);

        const SceneViewport sv = window.getSceneViewport();
        const auto [scaleX, scaleY] = window.getContentScale();
        const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
        const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
        const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
        const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);

        VkViewport viewport{};
        viewport.x = static_cast<float>(fbX);
        viewport.y = static_cast<float>(fbY);
        viewport.width = static_cast<float>(fbW);
        viewport.height = static_cast<float>(fbH);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{fbX, fbY}, {static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH)}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        glm::mat4 viewProj = camera.getProjectionMatrix() * cameraTransform.getViewMatrix();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        VkBuffer vbuf = vertexBuffers_[bufIdx].buffer;
        VkDeviceSize voffset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &voffset);
        vkCmdPushConstants(cmd,
                           pipeline_->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(glm::mat4),
                           &viewProj);

        // Draw each font group with its atlas descriptor
        uint32_t vertexOffset = 0;
        for (auto& [fontName, verts]: byFont) {
            if (verts.empty()) continue;
            Font* font = assetLoader_.get<Font>(fontName);
            if (!font) {
                vertexOffset += static_cast<uint32_t>(verts.size());
                continue;
            }

            VkDescriptorSet ds = getOrCreateDescriptor(*font);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->layout, 0, 1, &ds, 0, nullptr);
            vkCmdDraw(cmd, static_cast<uint32_t>(verts.size()), 1, vertexOffset, 0);
            vertexOffset += static_cast<uint32_t>(verts.size());
        }

        vkCmdEndRendering(cmd);
    }

private:
    void initPipeline() {
        const Shader* shader = assetLoader_.get<Shader>(TEXT_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(*shader, colorFormat_, depthFormat_);
    }

    void initVertexBuffers(uint32_t frameCount) {
        vertexBuffers_.resize(frameCount);
        const VkDeviceSize bufSize = MAX_QUADS * 6 * sizeof(TextVertex);
        for (auto& buf: vertexBuffers_) {
            buf = createBuffer(context_.device,
                               context_.physicalDevice,
                               bufSize,
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }

    VkDescriptorSet getOrCreateDescriptor(const Font& font) {
        auto it = descriptorCache_.find(font.name);
        if (it != descriptorCache_.end()) return it->second;

        VulkanTexture& tex = resourcesManager_.getFontAtlas(font);

        VkDescriptorSetLayout layout = pipeline_->descriptorSetLayouts[0];
        VkDescriptorSet ds = createEmptyDescriptorSet(context_.device, context_.descriptorPool, layout);

        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView = tex.imageView;
        imgInfo.sampler = tex.sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = ds;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imgInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);

        descriptorCache_.emplace(font.name, ds);
        return ds;
    }

    void buildQuads(const Font& font,
                    const TextDrawCall& call,
                    const Transform& cameraTransform,
                    std::vector<TextVertex>& out) const {
        const float scale = call.fontSize / font.fontSize / 100.0f;
        float cursorX = 0.0f;

        glm::vec3 right(1.0f, 0.0f, 0.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        if (call.billboard) {
            // Extract camera right and up from the inverse view matrix (= camera world matrix)
            glm::mat4 view = cameraTransform.getViewMatrix();
            glm::mat4 invView = glm::inverse(view);
            right = glm::vec3(invView[0]);
            up = glm::vec3(invView[1]);
        }

        // Measure total width for center/right alignment
        float totalWidth = 0.0f;
        if (call.alignment != TextAlignment::Left) {
            for (char c: call.text) {
                auto git = font.glyphs.find(static_cast<uint32_t>(c));
                totalWidth += (git != font.glyphs.end() ? git->second.advance : font.fontSize * 0.5f) * scale;
            }
        }
        if (call.alignment == TextAlignment::Center)
            cursorX = -totalWidth * 0.5f;
        else if (call.alignment == TextAlignment::Right)
            cursorX = -totalWidth;

        for (char c: call.text) {
            auto codepoint = static_cast<uint32_t>(c);
            auto git = font.glyphs.find(codepoint);
            if (git == font.glyphs.end()) {
                cursorX += font.fontSize * 0.5f * scale;
                continue;
            }
            const Font::Glyph& g = git->second;

            // bearingX/bearingY are stb xoff/yoff: pixel offsets from cursor to glyph top-left.
            // bearingY is negative for glyphs above baseline (most Latin glyphs).
            // In world space: right = +X, up = +Y, so negate Y to flip screen → world.
            const float x0 = cursorX + g.bearingX * scale;
            const float x1 = x0 + static_cast<float>(g.width) * scale;
            const float y0 = -g.bearingY * scale; // positive = glyph top above baseline
            const float y1 = y0 - static_cast<float>(g.height) * scale; // glyph bottom

            const glm::vec3 tl = call.worldPosition + right * x0 + up * y0;
            const glm::vec3 tr = call.worldPosition + right * x1 + up * y0;
            const glm::vec3 bl = call.worldPosition + right * x0 + up * y1;
            const glm::vec3 br = call.worldPosition + right * x1 + up * y1;

            out.push_back({tl, {g.u0, g.v0}, call.color});
            out.push_back({bl, {g.u0, g.v1}, call.color});
            out.push_back({tr, {g.u1, g.v0}, call.color});
            out.push_back({tr, {g.u1, g.v0}, call.color});
            out.push_back({bl, {g.u0, g.v1}, call.color});
            out.push_back({br, {g.u1, g.v1}, call.color});

            cursorX += g.advance * scale;
        }
    }

    const VulkanContext& context_;
    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;
    VkFormat colorFormat_;
    VkFormat depthFormat_;

    VulkanPipeline* pipeline_ = nullptr;
    std::vector<VulkanBuffer> vertexBuffers_;
    uint32_t nextBufferIndex_ = 0;
    std::unordered_map<std::string, VkDescriptorSet> descriptorCache_;
};
