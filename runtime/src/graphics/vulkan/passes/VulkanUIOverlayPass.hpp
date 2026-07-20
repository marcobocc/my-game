#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <volk.h>
#include "../../assets/Font.hpp"
#include "../../assets/Texture.hpp"
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/buffers.hpp"
#include "../core/utils/descriptors.hpp"
#include "../core/utils/structs.hpp"
#include "core/GameWindow.hpp"
#include "core/assets/AssetLoader.hpp"
#include "core/assets/BuiltinAssetNames.hpp"
#include "graphics/GameRenderData.hpp"

/*
    VulkanUIOverlayPass

    Draws the game UI (screen-space windows/widgets) on top of the scene.
    A single pass draws both textured quads and text so that panels and
    glyphs interleave correctly in z-order. Vertices arrive in logical
    scene-viewport pixels; an orthographic projection maps them to NDC.
*/
class VulkanUIOverlayPass {
public:
    static constexpr uint32_t MAX_QUADS = 8192;

    VulkanUIOverlayPass(const VulkanContext& context,
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
        initPipelines();
        initVertexBuffers(frameCount);
    }

    ~VulkanUIOverlayPass() {
        for (auto& buf: vertexBuffers_)
            destroyBuffer(context_.device, buf);
    }

    VulkanUIOverlayPass(const VulkanUIOverlayPass&) = delete;
    VulkanUIOverlayPass& operator=(const VulkanUIOverlayPass&) = delete;
    VulkanUIOverlayPass(VulkanUIOverlayPass&&) = delete;
    VulkanUIOverlayPass& operator=(VulkanUIOverlayPass&&) = delete;

    void record(VkCommandBuffer cmd,
                VkImageView colorView,
                VkExtent2D extent,
                const GameWindow& window,
                const std::vector<UIDrawCall>& uiQueue,
                VkImageView depthView) {
        if (quadPipeline_ == nullptr || textPipeline_ == nullptr || uiQueue.empty()) return;

        // Flatten every batch into one vertex buffer, remembering draw ranges
        struct Batch {
            bool isText;
            std::string key; // texture or font name
            uint32_t first;
            uint32_t count;
        };
        std::vector<UIVertex> allVerts;
        std::vector<Batch> batches;
        allVerts.reserve(MAX_QUADS * 6);

        for (const auto& call: uiQueue) {
            const auto first = static_cast<uint32_t>(allVerts.size());
            if (call.kind == UIDrawCall::Kind::Quads) {
                allVerts.insert(allVerts.end(), call.vertices.begin(), call.vertices.end());
                if (allVerts.size() > MAX_QUADS * 6) allVerts.resize(MAX_QUADS * 6);
                const auto count = static_cast<uint32_t>(allVerts.size()) - first;
                if (count > 0) batches.push_back({false, call.textureOrFont, first, count});
            } else {
                const std::string fontName =
                        call.textureOrFont.empty() ? std::string(DEFAULT_FONT) : call.textureOrFont;
                Font* font = assetLoader_.get<Font>(fontName);
                if (!font || call.text.empty()) continue;
                buildTextQuads(*font, call, allVerts);
                if (allVerts.size() > MAX_QUADS * 6) allVerts.resize(MAX_QUADS * 6);
                const auto count = static_cast<uint32_t>(allVerts.size()) - first;
                if (count > 0) batches.push_back({true, fontName, first, count});
            }
        }
        if (batches.empty()) return;

        const uint32_t bufIdx = nextBufferIndex_++ % static_cast<uint32_t>(vertexBuffers_.size());
        updateBuffer(context_.device, vertexBuffers_[bufIdx], allVerts.data(), allVerts.size() * sizeof(UIVertex));

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

        // Logical pixels -> NDC. In Vulkan NDC y=-1 is the top, which this
        // mapping assigns to UI y=0.
        const glm::mat4 proj =
                glm::ortho(0.0f, static_cast<float>(sv.width), 0.0f, static_cast<float>(sv.height), -1.0f, 1.0f);

        VkBuffer vbuf = vertexBuffers_[bufIdx].buffer;
        VkDeviceSize voffset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &voffset);

        VulkanPipeline* boundPipeline = nullptr;
        for (const auto& batch: batches) {
            VulkanPipeline* pipeline = batch.isText ? textPipeline_ : quadPipeline_;
            VkDescriptorSet ds = batch.isText ? getOrCreateFontDescriptor(batch.key, *pipeline)
                                              : getOrCreateTextureDescriptor(batch.key, *pipeline);
            if (ds == VK_NULL_HANDLE) continue;

            if (pipeline != boundPipeline) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
                vkCmdPushConstants(cmd,
                                   pipeline->layout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0,
                                   sizeof(glm::mat4),
                                   &proj);
                boundPipeline = pipeline;
            }
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &ds, 0, nullptr);
            vkCmdDraw(cmd, batch.count, 1, batch.first, 0);
        }

        vkCmdEndRendering(cmd);
    }

private:
    void initPipelines() {
        const Shader* quadShader = assetLoader_.get<Shader>(UI_SHADER);
        const Shader* textShader = assetLoader_.get<Shader>(UI_TEXT_SHADER);
        if (quadShader) quadPipeline_ = &resourcesManager_.getPipeline(*quadShader, colorFormat_, depthFormat_);
        if (textShader) textPipeline_ = &resourcesManager_.getPipeline(*textShader, colorFormat_, depthFormat_);
    }

    void initVertexBuffers(uint32_t frameCount) {
        vertexBuffers_.resize(frameCount);
        const VkDeviceSize bufSize = MAX_QUADS * 6 * sizeof(UIVertex);
        for (auto& buf: vertexBuffers_) {
            buf = createBuffer(context_.device,
                               context_.physicalDevice,
                               bufSize,
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }

    VkDescriptorSet createImageDescriptor(const VulkanTexture& tex, const VulkanPipeline& pipeline) {
        VkDescriptorSetLayout layout = pipeline.descriptorSetLayouts[0];
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
        return ds;
    }

    VkDescriptorSet getOrCreateTextureDescriptor(const std::string& textureName, const VulkanPipeline& pipeline) {
        const std::string name = textureName.empty() ? std::string(EMPTY_TEXTURE) : textureName;
        const std::string key = "tex:" + name;
        auto it = descriptorCache_.find(key);
        if (it != descriptorCache_.end()) return it->second;

        Texture* texture = assetLoader_.get<Texture>(name);
        if (!texture) return VK_NULL_HANDLE;
        VkDescriptorSet ds = createImageDescriptor(resourcesManager_.getTexture(*texture), pipeline);
        descriptorCache_.emplace(key, ds);
        return ds;
    }

    VkDescriptorSet getOrCreateFontDescriptor(const std::string& fontName, const VulkanPipeline& pipeline) {
        const std::string key = "font:" + fontName;
        auto it = descriptorCache_.find(key);
        if (it != descriptorCache_.end()) return it->second;

        Font* font = assetLoader_.get<Font>(fontName);
        if (!font) return VK_NULL_HANDLE;
        VkDescriptorSet ds = createImageDescriptor(resourcesManager_.getFontAtlas(*font), pipeline);
        descriptorCache_.emplace(key, ds);
        return ds;
    }

    // Screen-space glyph quads: origin = text baseline, y grows downward.
    static void buildTextQuads(const Font& font, const UIDrawCall& call, std::vector<UIVertex>& out) {
        const float scale = call.fontSize / font.fontSize;
        float cursorX = 0.0f;

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
            auto git = font.glyphs.find(static_cast<uint32_t>(c));
            if (git == font.glyphs.end()) {
                cursorX += font.fontSize * 0.5f * scale;
                continue;
            }
            const Font::Glyph& g = git->second;

            const float x0 = call.position.x + cursorX + g.bearingX * scale;
            const float x1 = x0 + static_cast<float>(g.width) * scale;
            const float y0 = call.position.y + g.bearingY * scale; // bearingY negative above baseline
            const float y1 = y0 + static_cast<float>(g.height) * scale;

            out.push_back({{x0, y0, 0.0f}, {g.u0, g.v0}, call.color});
            out.push_back({{x0, y1, 0.0f}, {g.u0, g.v1}, call.color});
            out.push_back({{x1, y0, 0.0f}, {g.u1, g.v0}, call.color});
            out.push_back({{x1, y0, 0.0f}, {g.u1, g.v0}, call.color});
            out.push_back({{x0, y1, 0.0f}, {g.u0, g.v1}, call.color});
            out.push_back({{x1, y1, 0.0f}, {g.u1, g.v1}, call.color});

            cursorX += g.advance * scale;
        }
    }

    const VulkanContext& context_;
    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;
    VkFormat colorFormat_;
    VkFormat depthFormat_;

    VulkanPipeline* quadPipeline_ = nullptr;
    VulkanPipeline* textPipeline_ = nullptr;
    std::vector<VulkanBuffer> vertexBuffers_;
    uint32_t nextBufferIndex_ = 0;
    std::unordered_map<std::string, VkDescriptorSet> descriptorCache_;
};
