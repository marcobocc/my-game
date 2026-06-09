#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <log4cxx/logger.h>
#include <unordered_map>
#include <vector>
#include <volk.h>
#include "../core/utils/buffers.hpp"
#include "../core/utils/error_handling.hpp"
#include "../core/utils/memory.hpp"
#include "../core/utils/shaders.hpp"
#include "../core/utils/structs.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/asset_management/asset_types/Shader.hpp"
#include "modules/asset_management/asset_types/Texture.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/rendering/GameRenderData.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanTextureCache.hpp"
#include "modules/scene/components/ParticleEmitter.hpp"

// Per-emitter GPU resources, allocated once when the emitter is first seen.
struct ParticleEmitterGpuState {
    VulkanBuffer particleBuffer; // GpuParticle[]        DEVICE_LOCAL SSBO
    VulkanBuffer freelistBuffer; // count + uint[]       DEVICE_LOCAL SSBO
    VulkanBuffer indirectBuffer; // VkDrawIndirectCommand DEVICE_LOCAL SSBO|INDIRECT
    std::vector<VulkanBuffer> configUBOs; // one per frame-in-flight HOST_VISIBLE UBO

    VkDescriptorSet computeSet0 = VK_NULL_HANDLE; // bindings 0,1: particle/freelist SSBOs
    std::vector<VkDescriptorSet> computeSet1s; // one per frame: emitter config UBO
    std::vector<VkDescriptorSet> graphicsSet1s; // one per frame: particle SSBO + sprite sampler + config UBO

    uint32_t maxParticles = 0;
    bool initialised = false;
    std::string boundSpriteTexture;
};

class VulkanParticlePass {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("VulkanParticlePass");

public:
    VulkanParticlePass(const VulkanContext& context,
                       AssetLoader& assetLoader,
                       VulkanTextureCache& textureCache,
                       uint32_t framesInFlight,
                       VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
                       VkFormat depthFormat = VK_FORMAT_D32_SFLOAT) :
        context_(context),
        assetLoader_(assetLoader),
        textureCache_(textureCache),
        framesInFlight_(framesInFlight),
        colorFormat_(colorFormat),
        depthFormat_(depthFormat) {
        createDescriptorSetLayouts();
        createFallbackTexture();
        loadPipelines(assetLoader);
    }

    ~VulkanParticlePass() {
        for (auto& [key, state]: emitterStates_)
            destroyEmitterState(state);
        if (fallbackTexture_.image != VK_NULL_HANDLE) {
            vkDestroyImageView(context_.device, fallbackTexture_.imageView, nullptr);
            vkDestroyImage(context_.device, fallbackTexture_.image, nullptr);
            vkFreeMemory(context_.device, fallbackTexture_.memory, nullptr);
            vkDestroySampler(context_.device, fallbackTexture_.sampler, nullptr);
        }

        if (computeSet0Layout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(context_.device, computeSet0Layout_, nullptr);
        if (configUBOLayout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(context_.device, configUBOLayout_, nullptr);
        if (graphicsParticleLayout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(context_.device, graphicsParticleLayout_, nullptr);

        if (spawnPipeline_.pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(context_.device, spawnPipeline_.pipeline, nullptr);
        if (spawnPipeline_.layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(context_.device, spawnPipeline_.layout, nullptr);

        if (simulatePipeline_.pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(context_.device, simulatePipeline_.pipeline, nullptr);
        if (simulatePipeline_.layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(context_.device, simulatePipeline_.layout, nullptr);

        if (drawPipeline_ != nullptr && drawPipeline_->pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(context_.device, drawPipeline_->pipeline, nullptr);
        if (drawPipeline_ != nullptr && drawPipeline_->layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(context_.device, drawPipeline_->layout, nullptr);
        delete drawPipeline_;
    }

    // Called each frame — spawn dispatch, barrier, simulate dispatch, barrier, graphics draw.
    void record(VkCommandBuffer cmd,
                const std::vector<ParticleEmitterRef>& emitters,
                uint32_t frameIndex,
                float deltaTime,
                VkImageView colorView,
                VkImageView depthView,
                VkExtent2D extent,
                VkDescriptorSet cameraDescriptorSet,
                const GameWindow* window = nullptr) {
        if (!spawnPipeline_.pipeline || !simulatePipeline_.pipeline || !drawPipeline_) return;

        for (const auto& ref: emitters) {
            if (!ref.emitter) continue;
            ParticleEmitterGpuState& state = getOrCreateState(*ref.emitter);

            uploadConfigUBO(state, *ref.emitter, ref.worldPosition, frameIndex);
            resetIndirectBuffer(cmd, state);

            // Spawn
            if (ref.toSpawn > 0) {
                struct SpawnPC {
                    uint32_t toSpawn;
                    uint32_t frameIndex;
                } pc{ref.toSpawn, frameIndex};
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, spawnPipeline_.pipeline);
                VkDescriptorSet sets[] = {state.computeSet0, state.computeSet1s[frameIndex]};
                vkCmdBindDescriptorSets(
                        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, spawnPipeline_.layout, 0, 2, sets, 0, nullptr);
                vkCmdPushConstants(cmd, spawnPipeline_.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
                vkCmdDispatch(cmd, (ref.toSpawn + 63u) / 64u, 1, 1);
            }

            // Barrier: spawn writes -> simulate reads particle + freelist buffers
            {
                VkBufferMemoryBarrier barriers[2] = {};
                barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[0].buffer = state.particleBuffer.buffer;
                barriers[0].offset = 0;
                barriers[0].size = VK_WHOLE_SIZE;
                barriers[1] = barriers[0];
                barriers[1].buffer = state.freelistBuffer.buffer;
                vkCmdPipelineBarrier(cmd,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     2,
                                     barriers,
                                     0,
                                     nullptr);
            }

            // Simulate
            {
                struct SimPC {
                    float deltaTime;
                    uint32_t maxParticles;
                } pc{deltaTime, state.maxParticles};
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, simulatePipeline_.pipeline);
                VkDescriptorSet sets[] = {state.computeSet0, state.computeSet1s[frameIndex]};
                vkCmdBindDescriptorSets(
                        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, simulatePipeline_.layout, 0, 2, sets, 0, nullptr);
                vkCmdPushConstants(cmd, simulatePipeline_.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
                vkCmdDispatch(cmd, (state.maxParticles + 63u) / 64u, 1, 1);
            }

            // Barrier: simulate writes -> vertex reads particle buffer + indirect draw
            {
                VkBufferMemoryBarrier barriers[2] = {};
                barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[0].buffer = state.particleBuffer.buffer;
                barriers[0].offset = 0;
                barriers[0].size = VK_WHOLE_SIZE;
                barriers[1] = barriers[0];
                barriers[1].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                barriers[1].buffer = state.indirectBuffer.buffer;
                vkCmdPipelineBarrier(cmd,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     2,
                                     barriers,
                                     0,
                                     nullptr);
            }

            // Graphics draw
            {
                VkRenderingAttachmentInfo colorAtt{};
                colorAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                colorAtt.imageView = colorView;
                colorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

                VkRenderingAttachmentInfo depthAtt{};
                depthAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
                depthAtt.imageView = depthView;
                depthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_NONE;

                VkRenderingInfo renderInfo{};
                renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
                renderInfo.renderArea = {{0, 0}, extent};
                renderInfo.layerCount = 1;
                renderInfo.colorAttachmentCount = 1;
                renderInfo.pColorAttachments = &colorAtt;
                renderInfo.pDepthAttachment = &depthAtt;
                vkCmdBeginRendering(cmd, &renderInfo);

                float vpX = 0.f, vpY = 0.f;
                float vpW = static_cast<float>(extent.width), vpH = static_cast<float>(extent.height);
                if (window) {
                    const SceneViewport sv = window->getSceneViewport();
                    const auto [scaleX, scaleY] = window->getContentScale();
                    vpX = static_cast<float>(sv.x) * scaleX;
                    vpY = static_cast<float>(sv.y) * scaleY;
                    vpW = static_cast<float>(sv.width) * scaleX;
                    vpH = static_cast<float>(sv.height) * scaleY;
                }
                VkViewport vp{vpX, vpY, vpW, vpH, 0.f, 1.f};
                vkCmdSetViewport(cmd, 0, 1, &vp);
                VkRect2D scissor{{static_cast<int32_t>(vpX), static_cast<int32_t>(vpY)},
                                 {static_cast<uint32_t>(vpW), static_cast<uint32_t>(vpH)}};
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                updateSpriteDescriptor(state, ref.emitter->spriteTexture);
                uint32_t hasSprite = ref.emitter->spriteTexture.empty() ? 0u : 1u;

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawPipeline_->pipeline);
                VkDescriptorSet sets[] = {cameraDescriptorSet, state.graphicsSet1s[frameIndex]};
                vkCmdBindDescriptorSets(
                        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, drawPipeline_->layout, 0, 2, sets, 0, nullptr);
                vkCmdPushConstants(
                        cmd, drawPipeline_->layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &hasSprite);
                vkCmdDrawIndirect(cmd, state.indirectBuffer.buffer, 0, 1, sizeof(VkDrawIndirectCommand));
                vkCmdEndRendering(cmd);
            }
        }
    }

private:
    // GPU layout of GpuParticle — must match shader struct exactly.
    struct GpuParticle {
        glm::vec3 position;
        float lifetime;
        glm::vec3 velocity;
        float maxLifetime;
        glm::vec4 colorStart;
        glm::vec4 colorEnd;
        float sizeStart;
        float sizeEnd;
        float rotation; // radians
        float _pad;
    };

    // Emitter config UBO — must match shader layout exactly (std140).
    struct EmitterConfigUBO {
        alignas(16) glm::vec3 emitterPos;
        float lifetimeMin;
        alignas(16) glm::vec3 gravity;
        float lifetimeMax;
        // Cone shape
        float coneAngle; // half-angle in radians
        float coneRadius;
        float speedMin;
        float speedMax;
        // Size
        float sizeStart;
        float sizeEnd;
        float _pad0;
        float _pad1;
        // Per-particle color random range
        alignas(16) glm::vec4 colorStartMin;
        alignas(16) glm::vec4 colorStartMax;
        alignas(16) glm::vec4 colorEndMin;
        alignas(16) glm::vec4 colorEndMax;
        // Rotation range (radians)
        float rotationMin;
        float rotationMax;
        int colorKeyCount;
        float _pad2;
        // Color over lifetime gradient (up to 8 stops)
        alignas(16) glm::vec4 gradientColors[8];
        alignas(16) glm::vec4 gradientTimes[8]; // .x = normalized time [0,1]
    };

    ParticleEmitterGpuState& getOrCreateState(ParticleEmitter& emitter) {
        auto it = emitterStates_.find(&emitter);
        if (it != emitterStates_.end()) return it->second;

        LOG4CXX_INFO(LOGGER,
                     "ParticlePass: allocating GPU state for new emitter ptr=" << &emitter << " maxParticles="
                                                                               << emitter.maxParticles);
        ParticleEmitterGpuState& state = emitterStates_[&emitter];
        state.maxParticles = emitter.maxParticles;
        allocateEmitterBuffers(state, emitter.maxParticles);
        allocateEmitterDescriptorSets(state);
        initialiseGpuBuffers(state, emitter.maxParticles);
        state.initialised = true;
        LOG4CXX_INFO(LOGGER, "ParticlePass: emitter GPU state ready (computeSet0=" << state.computeSet0 << ")");
        return state;
    }

    void allocateEmitterBuffers(ParticleEmitterGpuState& state, uint32_t maxParticles) {
        const VkDeviceSize particleSize = sizeof(GpuParticle) * maxParticles;
        // Freelist: one uint for count + maxParticles uint indices
        const VkDeviceSize freelistSize = sizeof(uint32_t) * (1 + maxParticles);
        const VkDeviceSize indirectSize = sizeof(VkDrawIndirectCommand);
        const VkDeviceSize configSize = sizeof(EmitterConfigUBO);

        state.particleBuffer = createBuffer(context_.device,
                                            context_.physicalDevice,
                                            particleSize,
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        state.freelistBuffer = createBuffer(context_.device,
                                            context_.physicalDevice,
                                            freelistSize,
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        state.indirectBuffer = createBuffer(context_.device,
                                            context_.physicalDevice,
                                            indirectSize,
                                            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        state.configUBOs.resize(framesInFlight_);
        for (auto& ubo: state.configUBOs)
            ubo = createBuffer(context_.device,
                               context_.physicalDevice,
                               configSize,
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void initialiseGpuBuffers(ParticleEmitterGpuState& state, uint32_t maxParticles) {
        // Staging buffer: write freelist initial data (count=maxParticles, indices[i]=i)
        // and particle lifetimes = -1 (dead).
        const VkDeviceSize particleSize = sizeof(GpuParticle) * maxParticles;
        const VkDeviceSize freelistSize = sizeof(uint32_t) * (1 + maxParticles);

        // Init particles: all lifetime = -1
        std::vector<GpuParticle> particles(maxParticles);
        for (auto& p: particles)
            p.lifetime = -1.0f;

        // Init freelist: count = maxParticles, then 0..maxParticles-1
        std::vector<uint32_t> freelist(1 + maxParticles);
        freelist[0] = maxParticles;
        for (uint32_t i = 0; i < maxParticles; ++i)
            freelist[1 + i] = i;

        uploadViaStagingBuffer(state.particleBuffer.buffer, particles.data(), particleSize);
        uploadViaStagingBuffer(state.freelistBuffer.buffer, freelist.data(), freelistSize);

        // Init indirect: {6, 0, 0, 0}
        VkDrawIndirectCommand indirect{6, 0, 0, 0};
        uploadViaStagingBuffer(state.indirectBuffer.buffer, &indirect, sizeof(indirect));
    }

    void uploadViaStagingBuffer(VkBuffer dst, const void* data, VkDeviceSize size) {
        VulkanBuffer staging = createBuffer(context_.device,
                                            context_.physicalDevice,
                                            size,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            data);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool_;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(context_.device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);
        VkBufferCopy region{0, 0, size};
        vkCmdCopyBuffer(cmd, staging.buffer, dst, 1, &region);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        vkQueueSubmit(context_.graphicsQueue, 1, &submit, VK_NULL_HANDLE);
        vkQueueWaitIdle(context_.graphicsQueue);
        vkFreeCommandBuffers(context_.device, commandPool_, 1, &cmd);
        destroyBuffer(context_.device, staging);
    }

    void allocateEmitterDescriptorSets(ParticleEmitterGpuState& state) {
        // Set 0 (compute): particle SSBO (binding 0), freelist SSBO (binding 1)
        {
            VkDescriptorSetAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            info.descriptorPool = context_.descriptorPool;
            info.descriptorSetCount = 1;
            info.pSetLayouts = &computeSet0Layout_;
            vkAllocateDescriptorSets(context_.device, &info, &state.computeSet0);

            VkDescriptorBufferInfo bufs[2] = {};
            bufs[0] = {state.particleBuffer.buffer, 0, VK_WHOLE_SIZE};
            bufs[1] = {state.freelistBuffer.buffer, 0, VK_WHOLE_SIZE};
            VkWriteDescriptorSet writes[2] = {};
            for (uint32_t i = 0; i < 2; ++i) {
                writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i].dstSet = state.computeSet0;
                writes[i].dstBinding = i;
                writes[i].descriptorCount = 1;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[i].pBufferInfo = &bufs[i];
            }
            vkUpdateDescriptorSets(context_.device, 2, writes, 0, nullptr);
        }

        // Set 1 (compute, per-frame): emitter config UBO
        state.computeSet1s.resize(framesInFlight_);
        for (uint32_t f = 0; f < framesInFlight_; ++f) {
            VkDescriptorSetAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            info.descriptorPool = context_.descriptorPool;
            info.descriptorSetCount = 1;
            info.pSetLayouts = &configUBOLayout_;
            vkAllocateDescriptorSets(context_.device, &info, &state.computeSet1s[f]);

            VkDescriptorBufferInfo bufInfo{state.configUBOs[f].buffer, 0, VK_WHOLE_SIZE};
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = state.computeSet1s[f];
            write.dstBinding = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = &bufInfo;
            vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);
        }

        // Set 1 (graphics, per-frame): binding 0 = particle SSBO, binding 1 = sprite sampler, binding 2 = config UBO
        state.graphicsSet1s.resize(framesInFlight_);
        for (uint32_t f = 0; f < framesInFlight_; ++f) {
            VkDescriptorSetAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            info.descriptorPool = context_.descriptorPool;
            info.descriptorSetCount = 1;
            info.pSetLayouts = &graphicsParticleLayout_;
            vkAllocateDescriptorSets(context_.device, &info, &state.graphicsSet1s[f]);

            VkDescriptorBufferInfo particleBufInfo{state.particleBuffer.buffer, 0, VK_WHOLE_SIZE};
            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfo.imageView = fallbackTexture_.imageView;
            imgInfo.sampler = fallbackTexture_.sampler;
            VkDescriptorBufferInfo configBufInfo{state.configUBOs[f].buffer, 0, VK_WHOLE_SIZE};

            VkWriteDescriptorSet writes[3] = {};
            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = state.graphicsSet1s[f];
            writes[0].dstBinding = 0;
            writes[0].descriptorCount = 1;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[0].pBufferInfo = &particleBufInfo;
            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = state.graphicsSet1s[f];
            writes[1].dstBinding = 1;
            writes[1].descriptorCount = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[1].pImageInfo = &imgInfo;
            writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[2].dstSet = state.graphicsSet1s[f];
            writes[2].dstBinding = 2;
            writes[2].descriptorCount = 1;
            writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[2].pBufferInfo = &configBufInfo;

            vkUpdateDescriptorSets(context_.device, 3, writes, 0, nullptr);
        }
    }

    void uploadConfigUBO(ParticleEmitterGpuState& state,
                         const ParticleEmitter& emitter,
                         const glm::vec3& worldPos,
                         uint32_t frameIndex) {
        EmitterConfigUBO ubo{};
        ubo.emitterPos = worldPos;
        ubo.lifetimeMin = emitter.lifetimeMin;
        ubo.lifetimeMax = emitter.lifetimeMax;
        ubo.gravity = emitter.gravity;
        ubo.coneAngle = glm::radians(emitter.coneAngle);
        ubo.coneRadius = emitter.coneRadius;
        ubo.speedMin = emitter.speedMin;
        ubo.speedMax = emitter.speedMax;
        ubo.sizeStart = emitter.sizeStart;
        ubo.sizeEnd = emitter.sizeEnd;
        ubo.colorStartMin = emitter.colorStartMin;
        ubo.colorStartMax = emitter.colorStartMax;
        ubo.colorEndMin = emitter.colorEndMin;
        ubo.colorEndMax = emitter.colorEndMax;
        ubo.rotationMin = glm::radians(emitter.rotationMin);
        ubo.rotationMax = glm::radians(emitter.rotationMax);
        ubo.colorKeyCount = emitter.colorKeyCount;
        for (int i = 0; i < emitter.colorKeyCount && i < 8; ++i) {
            ubo.gradientColors[i] = emitter.colorKeys[i].color;
            ubo.gradientTimes[i] = glm::vec4(emitter.colorKeys[i].time, 0.0f, 0.0f, 0.0f);
        }
        updateBuffer(context_.device, state.configUBOs[frameIndex], &ubo, sizeof(ubo));
    }

    void resetIndirectBuffer(VkCommandBuffer cmd, ParticleEmitterGpuState& state) {
        // Barrier: previous frame's indirect read → transfer write for reset
        {
            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = state.indirectBuffer.buffer;
            barrier.offset = 0;
            barrier.size = VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier,
                                 0,
                                 nullptr);
        }

        // Write {vertexCount=6, instanceCount=maxParticles, firstVertex=0, firstInstance=0}
        uint32_t indirectData[4] = {VERTEX_COUNT_6, state.maxParticles, 0u, 0u};
        vkCmdUpdateBuffer(cmd, state.indirectBuffer.buffer, 0, sizeof(indirectData), indirectData);

        // Barrier: transfer write → indirect read (no compute access needed since simulate doesn't touch it)
        {
            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = state.indirectBuffer.buffer;
            barrier.offset = 0;
            barrier.size = VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier,
                                 0,
                                 nullptr);
        }
    }

    void destroyEmitterState(ParticleEmitterGpuState& state) {
        destroyBuffer(context_.device, state.particleBuffer);
        destroyBuffer(context_.device, state.freelistBuffer);
        destroyBuffer(context_.device, state.indirectBuffer);
        for (auto& ubo: state.configUBOs)
            destroyBuffer(context_.device, ubo);
    }

    void createDescriptorSetLayouts() {
        // Create a transient command pool for one-time upload commands
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = context_.graphicsQueueFamilyIndex;
        VkResult poolResult = vkCreateCommandPool(context_.device, &poolInfo, nullptr, &commandPool_);
        if (poolResult != VK_SUCCESS) LOG4CXX_ERROR(LOGGER, "vkCreateCommandPool failed: " << poolResult);

        // computeSet0Layout: 2 storage buffers at bindings 0 (particles) and 1 (freelist)
        {
            VkDescriptorSetLayoutBinding bindings[2] = {};
            for (uint32_t i = 0; i < 2; ++i) {
                bindings[i].binding = i;
                bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                bindings[i].descriptorCount = 1;
                bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            }
            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.bindingCount = 2;
            info.pBindings = bindings;
            vkCreateDescriptorSetLayout(context_.device, &info, nullptr, &computeSet0Layout_);
        }

        // configUBOLayout: 1 uniform buffer at binding 0 (used by both compute shaders as set 1)
        {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = 0;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.bindingCount = 1;
            info.pBindings = &binding;
            vkCreateDescriptorSetLayout(context_.device, &info, nullptr, &configUBOLayout_);
        }

        // graphicsParticleLayout: binding 0 = particle SSBO (vertex), binding 1 = sprite sampler (fragment),
        //                          binding 2 = emitter config UBO (vertex, for gradient evaluation)
        {
            VkDescriptorSetLayoutBinding bindings[3] = {};
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[1].binding = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[1].descriptorCount = 1;
            bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings[2].binding = 2;
            bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bindings[2].descriptorCount = 1;
            bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.bindingCount = 3;
            info.pBindings = bindings;
            vkCreateDescriptorSetLayout(context_.device, &info, nullptr, &graphicsParticleLayout_);
        }
    }

    void loadPipelines(AssetLoader& assetLoader) {
        const Shader* spawnShader = assetLoader.get<Shader>(PARTICLES_SPAWN_SHADER);
        const Shader* simulateShader = assetLoader.get<Shader>(PARTICLES_SIMULATE_SHADER);
        const Shader* drawShader = assetLoader.get<Shader>(PARTICLES_SHADER);

        if (!spawnShader || spawnShader->computeBytecode.empty()) {
            LOG4CXX_ERROR(LOGGER,
                          "ParticlePass: spawn shader missing or has no compute bytecode (path="
                                  << PARTICLES_SPAWN_SHADER << ")");
            return;
        }
        if (!simulateShader || simulateShader->computeBytecode.empty()) {
            LOG4CXX_ERROR(LOGGER,
                          "ParticlePass: simulate shader missing or has no compute bytecode (path="
                                  << PARTICLES_SIMULATE_SHADER << ")");
            return;
        }
        if (!drawShader || drawShader->vertexBytecode.empty() || drawShader->fragmentBytecode.empty()) {
            LOG4CXX_ERROR(LOGGER, "ParticlePass: draw shader missing or incomplete (path=" << PARTICLES_SHADER << ")");
            return;
        }

        LOG4CXX_INFO(LOGGER, "ParticlePass: building compute pipelines...");
        spawnPipeline_ = buildComputePipeline(
                spawnShader->computeBytecode, {computeSet0Layout_, configUBOLayout_}, sizeof(uint32_t) * 2);
        simulatePipeline_ = buildComputePipeline(simulateShader->computeBytecode,
                                                 {computeSet0Layout_, configUBOLayout_},
                                                 sizeof(float) + sizeof(uint32_t));
        LOG4CXX_INFO(LOGGER, "ParticlePass: building graphics pipeline...");
        drawPipeline_ = buildGraphicsPipeline(*drawShader);
        LOG4CXX_INFO(LOGGER,
                     "ParticlePass: pipelines ready (spawn=" << spawnPipeline_.pipeline
                                                             << " sim=" << simulatePipeline_.pipeline
                                                             << " draw=" << drawPipeline_->pipeline << ")");
    }

    struct ComputePipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
    };

    ComputePipeline buildComputePipeline(const std::vector<char>& spirv,
                                         const std::vector<VkDescriptorSetLayout>& setLayouts,
                                         uint32_t pushConstantSize) {
        auto [module, stageInfo] = createShader(context_.device, spirv, VK_SHADER_STAGE_COMPUTE_BIT);

        VkPushConstantRange pcRange{};
        pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pcRange.offset = 0;
        pcRange.size = pushConstantSize;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();
        layoutInfo.pushConstantRangeCount = pushConstantSize > 0 ? 1 : 0;
        layoutInfo.pPushConstantRanges = pushConstantSize > 0 ? &pcRange : nullptr;

        ComputePipeline out{};
        VkResult r = vkCreatePipelineLayout(context_.device, &layoutInfo, nullptr, &out.layout);
        if (r != VK_SUCCESS) {
            LOG4CXX_ERROR(LOGGER, "vkCreatePipelineLayout failed: " << r);
            vkDestroyShaderModule(context_.device, module, nullptr);
            return out;
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = stageInfo;
        pipelineInfo.layout = out.layout;
        r = vkCreateComputePipelines(context_.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &out.pipeline);
        if (r != VK_SUCCESS) LOG4CXX_ERROR(LOGGER, "vkCreateComputePipelines failed: " << r);

        vkDestroyShaderModule(context_.device, module, nullptr);
        return out;
    }

    VulkanPipeline* buildGraphicsPipeline(const Shader& shader) {
        auto* p = new VulkanPipeline{};

        // Set layouts: set 0 = camera UBO (from geometry pass), set 1 = particle SSBO
        p->descriptorSetLayouts = {VK_NULL_HANDLE, graphicsParticleLayout_};
        // Build layout using camera-UBO layout as set 0.
        // We need a UBO layout for set 0 that matches the geometry pass camera UBO.
        VkDescriptorSetLayoutBinding camBinding{};
        camBinding.binding = 0;
        camBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        camBinding.descriptorCount = 1;
        camBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        VkDescriptorSetLayoutCreateInfo camLayoutInfo{};
        camLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        camLayoutInfo.bindingCount = 1;
        camLayoutInfo.pBindings = &camBinding;
        vkCreateDescriptorSetLayout(context_.device, &camLayoutInfo, nullptr, &cameraUBOLayout_);

        VkDescriptorSetLayout graphicsLayouts[] = {cameraUBOLayout_, graphicsParticleLayout_};

        VkPushConstantRange pcRange{};
        pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pcRange.offset = 0;
        pcRange.size = sizeof(uint32_t);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 2;
        layoutInfo.pSetLayouts = graphicsLayouts;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pcRange;
        VkResult r = vkCreatePipelineLayout(context_.device, &layoutInfo, nullptr, &p->layout);
        if (r != VK_SUCCESS) {
            LOG4CXX_ERROR(LOGGER, "Graphics pipeline layout creation failed: " << r);
            delete p;
            return nullptr;
        }

        auto [vertModule, vertStage] = createShader(context_.device, shader.vertexBytecode, VK_SHADER_STAGE_VERTEX_BIT);
        auto [fragModule, fragStage] =
                createShader(context_.device, shader.fragmentBytecode, VK_SHADER_STAGE_FRAGMENT_BIT);
        VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynStates;

        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
        raster.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo msaa{};
        msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth.depthTestEnable = VK_TRUE;
        depth.depthWriteEnable = VK_FALSE;
        depth.depthCompareOp = VK_COMPARE_OP_LESS;

        VkPipelineColorBlendAttachmentState blend{};
        blend.blendEnable = VK_TRUE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend.alphaBlendOp = VK_BLEND_OP_ADD;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                               VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blendState{};
        blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendState.attachmentCount = 1;
        blendState.pAttachments = &blend;

        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &colorFormat_;
        renderingInfo.depthAttachmentFormat = depthFormat_;
        LOG4CXX_INFO(LOGGER, "ParticlePass: graphics pipeline colorFormat=" << colorFormat_);

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &renderingInfo;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &raster;
        pipelineInfo.pMultisampleState = &msaa;
        pipelineInfo.pDepthStencilState = &depth;
        pipelineInfo.pColorBlendState = &blendState;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = p->layout;
        r = vkCreateGraphicsPipelines(context_.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &p->pipeline);
        if (r != VK_SUCCESS) LOG4CXX_ERROR(LOGGER, "vkCreateGraphicsPipelines failed: " << r);

        vkDestroyShaderModule(context_.device, vertModule, nullptr);
        vkDestroyShaderModule(context_.device, fragModule, nullptr);
        return p;
    }

    void createFallbackTexture() {
        // 1x1 opaque white RGBA8 texture used when no sprite is assigned
        Texture fallbackAsset("__particle_fallback__", 1, 1, 4, {0xFF, 0xFF, 0xFF, 0xFF});
        fallbackTexture_ = textureCache_.get(fallbackAsset);
    }

    void updateSpriteDescriptor(ParticleEmitterGpuState& state, const std::string& spritePath) {
        if (state.boundSpriteTexture == spritePath) return;

        const VulkanTexture* tex = nullptr;
        if (!spritePath.empty()) {
            const Texture* asset = assetLoader_.get<Texture>(spritePath);
            if (asset) tex = &textureCache_.get(*asset);
        }
        if (!tex) tex = &fallbackTexture_;

        VkDescriptorImageInfo imgInfo{};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView = tex->imageView;
        imgInfo.sampler = tex->sampler;

        for (VkDescriptorSet ds: state.graphicsSet1s) {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = ds;
            write.dstBinding = 1;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &imgInfo;
            vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);
        }

        state.boundSpriteTexture = spritePath;
    }

    static constexpr uint32_t VERTEX_COUNT_6 = 6;

    const VulkanContext& context_;
    AssetLoader& assetLoader_;
    VulkanTextureCache& textureCache_;
    uint32_t framesInFlight_;
    VkFormat colorFormat_;
    VkFormat depthFormat_;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;

    VkDescriptorSetLayout computeSet0Layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout configUBOLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout graphicsParticleLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout cameraUBOLayout_ = VK_NULL_HANDLE;

    VulkanTexture fallbackTexture_{};

    ComputePipeline spawnPipeline_;
    ComputePipeline simulatePipeline_;
    VulkanPipeline* drawPipeline_ = nullptr;

    std::unordered_map<ParticleEmitter*, ParticleEmitterGpuState> emitterStates_;
};
