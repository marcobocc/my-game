#include "VulkanBackend.hpp"
#include <functional>
#include <imgui_impl_vulkan.h>
#include <volk.h>
#include "../../../../runtime/src/graphics/RenderTargetHandle.hpp"
#include "core/GameWindow.hpp"
#include "core/assets/AssetLoader.hpp"
#include "graphics/vulkan/core/VulkanFrameManager.hpp"
#include "graphics/vulkan/core/VulkanSwapchainManager.hpp"
#include "graphics/vulkan/core/resources/VulkanRenderTargetManager.hpp"
#include "graphics/vulkan/core/resources/VulkanResourcesManager.hpp"
#include "graphics/vulkan/core/utils/buffers.hpp"
#include "graphics/vulkan/passes/VulkanGeometryPass.hpp"
#include "graphics/vulkan/passes/VulkanGizmoPass.hpp"
#include "graphics/vulkan/passes/VulkanGridPass.hpp"
#include "graphics/vulkan/passes/VulkanLightingPass.hpp"
#include "graphics/vulkan/passes/VulkanObjectIdPass.hpp"
#include "graphics/vulkan/passes/VulkanOutlinePass.hpp"
#include "graphics/vulkan/passes/VulkanParticlePass.hpp"
#include "graphics/vulkan/passes/VulkanShadowPass.hpp"
#include "graphics/vulkan/passes/VulkanSkyPass.hpp"
#include "graphics/vulkan/passes/VulkanTextPass.hpp"
#include "graphics/vulkan/passes/VulkanUIPass.hpp"

VulkanBackend::VulkanBackend(GameWindow& window,
                             VulkanContext& context,
                             VulkanFrameManager& frameManager,
                             VulkanRenderTargetManager& renderTargetManager,
                             VulkanGeometryPass& geometryPass,
                             VulkanLightingPass& lightingPass,
                             VulkanGridPass& gridPass,
                             VulkanGizmoPass& gizmoPass,
                             VulkanGizmoPass& gizmoOverlayPass,
                             VulkanObjectIdPass& objectIdPass,
                             VulkanOutlinePass& outlinePass,
                             VulkanUIPass& uiPass,
                             VulkanSwapchainManager& swapchainManager,
                             RendererSettings& settings,
                             VulkanResourcesManager& resourcesManager,
                             AssetLoader& assetLoader) :
    window_(window),
    context_(context),
    settings_(settings),
    swapchainManager_(swapchainManager),
    frameManager_(frameManager),
    renderTargetManager_(renderTargetManager),
    geometryPass_(geometryPass),
    lightingPass_(lightingPass),
    shadowPass_(std::make_unique<VulkanShadowPass>(resourcesManager, assetLoader, context)),
    skyPass_(std::make_unique<VulkanSkyPass>(context, assetLoader, resourcesManager)),
    particlePass_(std::make_unique<VulkanParticlePass>(
            context,
            assetLoader,
            resourcesManager.getTextureCache(),
            static_cast<uint32_t>(swapchainManager.swapchain().swapchainImages.size()),
            swapchainManager.swapchain().swapchainImageFormat,
            VK_FORMAT_D32_SFLOAT)),
    textPass_(
            std::make_unique<VulkanTextPass>(context,
                                             assetLoader,
                                             resourcesManager,
                                             static_cast<uint32_t>(swapchainManager.swapchain().swapchainImages.size()),
                                             swapchainManager.swapchain().swapchainImageFormat,
                                             VK_FORMAT_D32_SFLOAT)),
    gridPass_(gridPass),
    gizmoPass_(gizmoPass),
    gizmoOverlayPass_(gizmoOverlayPass),
    objectIdPass_(objectIdPass),
    outlinePass_(outlinePass),
    uiPass_(uiPass),
    resourcesManager_(resourcesManager),
    frameCount_(static_cast<uint32_t>(swapchainManager.swapchain().swapchainImages.size())),
    swapchainColorFormat_(swapchainManager.swapchain().swapchainImageFormat) {
    const auto& sc = swapchainManager_.swapchain();
    setupRenderGraph(sc.swapchainImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, sc.swapchainExtent);

    // Allocate per-frame resources for main swapchain render
    constexpr uint32_t MAX_SHADOW_LIGHTS = VulkanShadowPass::MAX_SHADOW_LIGHTS;
    mainLightingDescriptorSets_.resize(frameCount_);
    mainOutlineDescriptorSets_.resize(frameCount_);
    mainGeometryDescriptorSets_.resize(frameCount_);
    mainGeometryBuffers_.resize(frameCount_);
    for (uint32_t i = 0; i < frameCount_; ++i) {
        auto [buf, gds] = geometryPass_.createRenderContext();
        mainGeometryBuffers_[i] = std::move(buf);
        mainGeometryDescriptorSets_[i] = gds;
        for (uint32_t s = 0; s < MAX_SHADOW_LIGHTS; ++s)
            mainLightingDescriptorSets_[i][s] = lightingPass_.createDescriptorSet(s);
        mainOutlineDescriptorSets_[i] = outlinePass_.createDescriptorSet();
    }

    window_.onWindowResize([this](int, int, int, int) {
        swapchainManager_.recreate(context_);
        const auto& extent = swapchainManager_.swapchain().swapchainExtent;
        renderGraph_->onResize(extent.width, extent.height);
    });
}

VulkanBackend::~VulkanBackend() {
    vkDeviceWaitIdle(context_.device);

    // Cleanup off-screen render targets
    const auto& allRenderTargets = renderTargetManager_.getAllRenderTargets();
    for (size_t i = 0; i < allRenderTargets.size(); ++i) {
        RenderTargetHandle handle{static_cast<uint32_t>(i)};

        // Cleanup ImGui descriptor set
        auto imgit = renderTargetImGuiSets_.find(handle.index);
        if (imgit != renderTargetImGuiSets_.end() && imgit->second != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(imgit->second);
        }

        // Cleanup per-frame resources
        auto bit = renderTargetGeometryBuffers_.find(handle.index);
        if (bit != renderTargetGeometryBuffers_.end()) {
            for (auto& buf: bit->second)
                geometryPass_.destroyRenderContext(buf);
        }
    }
    renderTargetManager_.destroyAll();
    renderTargetImGuiSets_.clear();
    renderTargetLightingDescriptorSets_.clear();
    renderTargetOutlineDescriptorSets_.clear();
    renderTargetGeometryDescriptorSets_.clear();
    renderTargetGeometryBuffers_.clear();

    // Cleanup main swapchain resources
    for (auto& buf: mainGeometryBuffers_)
        geometryPass_.destroyRenderContext(buf);
}

// ------------------- Public API -------------------

void VulkanBackend::setDrawCallback(std::function<void()> callback) { uiPass_.setDrawCallback(std::move(callback)); }

bool VulkanBackend::renderFrame(const GameRenderData& rd) {
    static const std::vector<DrawCall> empty;
    static const std::vector<VulkanGizmoPass::GizmoVertex> noGizmos;
    return renderFrame(EditorRenderData{rd.camera,
                                        rd.cameraTransform,
                                        rd.drawQueue,
                                        empty,
                                        noGizmos,
                                        noGizmos,
                                        rd.lightsWithTransforms,
                                        rd.particleEmitters,
                                        rd.textQueue,
                                        1.0f,
                                        true});
}

bool VulkanBackend::renderFrame(const EditorRenderData& renderData) {
    // Off-screen cameras: queue up work to be batched into the next swapchain frame.
    if (renderData.camera.renderTarget.isValid()) {
        if (renderTargetManager_.get(renderData.camera.renderTarget))
            pendingOffscreenCameras_.push_back({renderData.camera, renderData.cameraTransform});
        return true;
    }

    flushOffscreenScenes();

    frameManager_.waitForCurrentFrame();

    uint32_t imageIndex = 0;
    if (!frameManager_.acquireImage(imageIndex)) return false;
    VkCommandBuffer cmd = frameManager_.beginFrame();
    recordCommands(cmd, imageIndex, renderData);
    frameManager_.endFrame(cmd);
    frameManager_.submit(imageIndex);
    frameManager_.advanceFrame();
    return true;
}

RenderTargetHandle VulkanBackend::createRenderTarget(uint32_t width, uint32_t height) {
    RenderTargetHandle handle = renderTargetManager_.create(width, height, swapchainColorFormat_);
    if (!handle.isValid()) return handle;

    auto* rt = renderTargetManager_.get(handle);
    if (!rt) return handle;

    // Allocate per-frame resources for this render target
    constexpr uint32_t MAX_SHADOW_LIGHTS = VulkanShadowPass::MAX_SHADOW_LIGHTS;
    auto& lightingDs = renderTargetLightingDescriptorSets_[handle.index];
    auto& outlineDs = renderTargetOutlineDescriptorSets_[handle.index];
    auto& geometryDs = renderTargetGeometryDescriptorSets_[handle.index];
    auto& geometryBufs = renderTargetGeometryBuffers_[handle.index];

    lightingDs.resize(frameCount_);
    outlineDs.resize(frameCount_);
    geometryDs.resize(frameCount_);
    geometryBufs.resize(frameCount_);

    for (uint32_t i = 0; i < frameCount_; ++i) {
        auto [buf, gds] = geometryPass_.createRenderContext();
        geometryBufs[i] = std::move(buf);
        geometryDs[i] = gds;
        for (uint32_t s = 0; s < MAX_SHADOW_LIGHTS; ++s)
            lightingDs[i][s] = lightingPass_.createDescriptorSet(s);
        outlineDs[i] = outlinePass_.createDescriptorSet();
    }

    // Register ImGui descriptor set for UI preview
    VkDescriptorSet imguiDescriptorSet =
            ImGui_ImplVulkan_AddTexture(rt->colorSampler, rt->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    renderTargetImGuiSets_[handle.index] = imguiDescriptorSet;
    return handle;
}

void VulkanBackend::destroyRenderTarget(RenderTargetHandle handle) {
    vkDeviceWaitIdle(context_.device);

    // Cleanup ImGui descriptor set
    auto it = renderTargetImGuiSets_.find(handle.index);
    if (it != renderTargetImGuiSets_.end() && it->second != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(it->second);
        renderTargetImGuiSets_.erase(it);
    }

    // Cleanup per-frame resources
    auto bit = renderTargetGeometryBuffers_.find(handle.index);
    if (bit != renderTargetGeometryBuffers_.end()) {
        for (auto& buf: bit->second)
            geometryPass_.destroyRenderContext(buf);
        renderTargetGeometryBuffers_.erase(bit);
    }

    renderTargetLightingDescriptorSets_.erase(handle.index);
    renderTargetOutlineDescriptorSets_.erase(handle.index);
    renderTargetGeometryDescriptorSets_.erase(handle.index);

    renderTargetManager_.destroy(handle);
}

VkDescriptorSet VulkanBackend::getRenderTargetImGuiId(RenderTargetHandle handle) const {
    auto it = renderTargetImGuiSets_.find(handle.index);
    return (it != renderTargetImGuiSets_.end()) ? it->second : VK_NULL_HANDLE;
}

VulkanRenderTarget VulkanBackend::getRenderTarget(RenderTargetHandle handle) const {
    const auto* rt = renderTargetManager_.get(handle);
    if (!rt) {
        return {0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_FORMAT_UNDEFINED};
    }
    return *rt;
}

std::vector<unsigned char> VulkanBackend::readbackRenderTarget(RenderTargetHandle handle) {
    const auto* rt = renderTargetManager_.get(handle);
    if (!rt || rt->image == VK_NULL_HANDLE) return {};

    // Editor-only, once-per-thumbnail cost: a coarse wait keeps this simple.
    vkQueueWaitIdle(context_.graphicsQueue);

    const VkDeviceSize size = static_cast<VkDeviceSize>(rt->width) * rt->height * 4;
    VulkanBuffer staging = createBuffer(context_.device,
                                        context_.physicalDevice,
                                        size,
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = context_.graphicsQueueFamilyIndex;
    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(context_.device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        destroyBuffer(context_.device, staging);
        return {};
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(context_.device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier toSrc{};
    toSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toSrc.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.image = rt->image;
    toSrc.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toSrc.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    toSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &toSrc);

    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {rt->width, rt->height, 1};
    vkCmdCopyImageToBuffer(cmd, rt->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging.buffer, 1, &region);

    VkImageMemoryBarrier back = toSrc;
    back.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    back.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    back.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    back.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &back);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(context_.graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(context_.graphicsQueue);
    vkDestroyCommandPool(context_.device, pool, nullptr);

    std::vector<unsigned char> pixels(size);
    void* mapped = nullptr;
    vkMapMemory(context_.device, staging.memory, 0, size, 0, &mapped);
    std::memcpy(pixels.data(), mapped, size);
    vkUnmapMemory(context_.device, staging.memory);
    destroyBuffer(context_.device, staging);

    // Render targets use the swapchain color format, which is commonly BGRA.
    if (rt->format == VK_FORMAT_B8G8R8A8_UNORM || rt->format == VK_FORMAT_B8G8R8A8_SRGB) {
        for (size_t i = 0; i + 3 < pixels.size(); i += 4)
            std::swap(pixels[i], pixels[i + 2]);
    }
    return pixels;
}

// ------------------- Frame helpers -------------------

void VulkanBackend::executeRenderGraph(VkCommandBuffer cmd,
                                       VkImage colorImage,
                                       VkImageView colorView,
                                       VkExtent2D extent,
                                       const EditorRenderData& renderData) {
    renderGraph_->resetAllResourceLayouts();
    renderGraph_->setImportedImage(colorTargetHandle_, colorImage, colorView, extent);
    renderGraph_->execute(cmd, renderData);
}

// Transition a color image from COLOR_ATTACHMENT_OPTIMAL to its final consumer layout.
static void transitionColorImageFinal(VkCommandBuffer cmd,
                                      VkImage image,
                                      VkImageLayout newLayout,
                                      VkPipelineStageFlags dstStage,
                                      VkAccessFlags dstAccess) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = dstAccess;
    vkCmdPipelineBarrier(
            cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

// Render one off-screen frame into renderData.camera.renderTarget, using the render
// target's per-frame descriptor sets, and leave the image in SHADER_READ_ONLY_OPTIMAL.
void VulkanBackend::recordOffscreenRender(VkCommandBuffer cmd, const EditorRenderData& renderData) {
    if (!renderData.camera.renderTarget.isValid()) return;
    const auto* target = renderTargetManager_.get(renderData.camera.renderTarget);
    if (!target || target->image == VK_NULL_HANDLE) return;

    size_t frameIdx = frameManager_.currentFrameIndex();
    uint32_t handleIdx = renderData.camera.renderTarget.index;

    auto bit = renderTargetGeometryBuffers_.find(handleIdx);
    if (bit == renderTargetGeometryBuffers_.end()) return;

    const auto& geomBufs = bit->second;
    currentFrameGeometryBuffer_ = (frameIdx < geomBufs.size()) ? const_cast<VulkanBuffer*>(&geomBufs[frameIdx])
                                                               : const_cast<VulkanBuffer*>(&geomBufs[0]);

    // Set current frame descriptor sets for render passes
    auto oit = renderTargetOutlineDescriptorSets_.find(handleIdx);
    auto git = renderTargetGeometryDescriptorSets_.find(handleIdx);
    if (oit != renderTargetOutlineDescriptorSets_.end()) {
        currentFrameOutlineDescriptorSet_ = (frameIdx < oit->second.size()) ? oit->second[frameIdx] : oit->second[0];
    }
    if (git != renderTargetGeometryDescriptorSets_.end()) {
        currentFrameGeometryDescriptorSet_ = (frameIdx < git->second.size()) ? git->second[frameIdx] : git->second[0];
    }

    executeRenderGraph(cmd, target->image, target->view, {target->width, target->height}, renderData);
    transitionColorImageFinal(cmd,
                              target->image,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                              VK_ACCESS_SHADER_READ_BIT);
}

// Render queued self-contained off-screen scenes (e.g. material previews), each in its
// own immediate submission. This cannot share the main frame's command buffer: the
// lighting pass uploads light data to shared host-visible SSBOs at record time, so the
// main scene's lights would overwrite the preview's before the GPU executes it.
void VulkanBackend::flushOffscreenScenes() {
    if (pendingOffscreenScenes_.empty()) return;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = context_.graphicsQueueFamilyIndex;
    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(context_.device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        pendingOffscreenScenes_.clear();
        return;
    }

    static const std::vector<DrawCall> noDrawCalls;
    static const std::vector<VulkanGizmoPass::GizmoVertex> noGizmos;
    for (auto& job: pendingOffscreenScenes_) {
        // In-flight frames share the render graph's transient images and light SSBOs.
        vkQueueWaitIdle(context_.graphicsQueue);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        vkAllocateCommandBuffers(context_.device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        recordOffscreenRender(cmd,
                              EditorRenderData{job.camera,
                                               job.cameraTransform,
                                               job.drawQueue,
                                               noDrawCalls,
                                               noGizmos,
                                               noGizmos,
                                               job.lights,
                                               {},
                                               {},
                                               1.0f,
                                               true,
                                               false});

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        vkQueueSubmit(context_.graphicsQueue, 1, &submit, VK_NULL_HANDLE);
        vkQueueWaitIdle(context_.graphicsQueue);
        vkFreeCommandBuffers(context_.device, pool, 1, &cmd);
    }
    pendingOffscreenScenes_.clear();
    vkDestroyCommandPool(context_.device, pool, nullptr);
}

void VulkanBackend::recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const EditorRenderData& renderData) {
    uiPass_.prepareFrame();

    // Flush off-screen cameras first, within the same command buffer.
    for (auto& job: pendingOffscreenCameras_) {
        recordOffscreenRender(cmd,
                              EditorRenderData{job.camera,
                                               job.cameraTransform,
                                               renderData.drawQueue,
                                               renderData.outlineQueue,
                                               renderData.gizmoLines,
                                               renderData.overlayGizmoLines,
                                               renderData.lightsWithTransforms,
                                               renderData.particleEmitters,
                                               renderData.textQueue,
                                               renderData.gridScale,
                                               true});
    }
    pendingOffscreenCameras_.clear();

    VulkanRenderTarget swapchainTarget = swapchainManager_.getSwapchainTarget(imageIndex);
    size_t frameIdx = frameManager_.currentFrameIndex();

    // Set current frame descriptor sets and buffer for render passes
    currentFrameOutlineDescriptorSet_ = mainOutlineDescriptorSets_[frameIdx];
    currentFrameGeometryDescriptorSet_ = mainGeometryDescriptorSets_[frameIdx];
    currentFrameGeometryBuffer_ = &mainGeometryBuffers_[frameIdx];

    executeRenderGraph(cmd,
                       swapchainTarget.image,
                       swapchainTarget.view,
                       {swapchainTarget.width, swapchainTarget.height},
                       renderData);

    transitionColorImageFinal(
            cmd, swapchainTarget.image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0);
}

bool VulkanBackend::renderUIOnly() {
    frameManager_.waitForCurrentFrame();

    uint32_t imageIndex = 0;
    if (!frameManager_.acquireImage(imageIndex)) return false;
    VkCommandBuffer cmd = frameManager_.beginFrame();

    uiPass_.prepareFrame();

    VulkanRenderTarget swapchainTarget = swapchainManager_.getSwapchainTarget(imageIndex);
    VkExtent2D extent{swapchainTarget.width, swapchainTarget.height};

    // Transition UNDEFINED → COLOR_ATTACHMENT_OPTIMAL (no TRANSFER_DST_BIT needed)
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = swapchainTarget.image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    // Use LOAD_OP_CLEAR inside the UI pass to produce a black background
    uiPass_.record(cmd, swapchainTarget.view, extent, /*clear=*/true);

    transitionColorImageFinal(
            cmd, swapchainTarget.image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0);

    frameManager_.endFrame(cmd);
    frameManager_.submit(imageIndex);
    frameManager_.advanceFrame();
    return true;
}

// ------------------- Render graph setup -------------------

void VulkanBackend::setupRenderGraph(VkFormat colorFormat, VkImageUsageFlags colorUsage, VkExtent2D extent) {
    renderGraph_.emplace(context_, extent);

    colorTargetHandle_ = renderGraph_->importImage("swapchain_color", colorFormat, colorUsage);

    constexpr uint32_t MAX_SHADOW_LIGHTS = VulkanShadowPass::MAX_SHADOW_LIGHTS;

    // --- N shadow cubemap images (fixed resolution, depth-only, 6 layers)
    // Each slot is a cube-compatible image. Directional/spot lights render into face 0 only;
    // point lights render into all 6 faces. The lighting shader samples the cube at binding 5.
    for (uint32_t s = 0; s < MAX_SHADOW_LIGHTS; ++s) {
        shadowDepthHandles_[s] = renderGraph_->createTransientCubemap("shadow_depth_" + std::to_string(s),
                                                                      VK_FORMAT_D32_SFLOAT,
                                                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                                              VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                      VulkanShadowPass::SHADOW_MAP_SIZE);
    }

    // --- N shadow passes (run first, one per light slot)
    // Each shadow resource is a cubemap; all 6 faces are declared as writes so the render graph
    // emits the correct layout barrier (UNDEFINED -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL) covering
    // all layers before we start rendering into them.
    for (uint32_t s = 0; s < MAX_SHADOW_LIGHTS; ++s) {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "ShadowPass_" + std::to_string(s);
        n.writes = {{shadowDepthHandles_[s],
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                     VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.execute = [this, s](VkCommandBuffer cmd,
                              const VulkanRenderGraph<EditorRenderData>& graph,
                              const EditorRenderData& ctx) -> bool {
            if (s >= ctx.lightsWithTransforms.size()) return false;
            const auto& [light, transform] = ctx.lightsWithTransforms[s];
            const bool castsShadows = light.castShadows;
            static const std::vector<DrawCall> emptyQueue{};

            if (light.type == LightType::POINT) {
                std::array<VkImageView, 6> faceViews{};
                for (uint32_t face = 0; face < 6; ++face)
                    faceViews[face] = graph.getFaceView(shadowDepthHandles_[s], face);
                shadowPass_->recordPointLight(
                        cmd, faceViews, castsShadows ? ctx.drawQueue : emptyQueue, ctx.lightsWithTransforms, s);
            } else {
                // Directional / spot: render into face 0 of the cubemap.
                shadowPass_->record(cmd,
                                    graph.getFaceView(shadowDepthHandles_[s], 0),
                                    castsShadows ? ctx.drawQueue : emptyQueue,
                                    ctx.lightsWithTransforms,
                                    ctx.camera,
                                    ctx.cameraTransform,
                                    s);
            }
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    objectIdColorHandle_ = renderGraph_->createTransientImage(
            "objectid_color", VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    // --- ObjectId pass ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "ObjectIdPass";
        n.writes = {
                {objectIdColorHandle_,
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                 VK_IMAGE_ASPECT_COLOR_BIT},
        };
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (ctx.isOffscreen) return false;
            objectIdMap_ = objectIdPass_.record(cmd,
                                                ctx.outlineQueue,
                                                ctx.camera,
                                                ctx.cameraTransform,
                                                window_,
                                                graph.getImageView(objectIdColorHandle_),
                                                graph.getWidth(objectIdColorHandle_),
                                                graph.getHeight(objectIdColorHandle_));
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    gbufferAlbedoHandle_ =
            renderGraph_->createTransientImage("gbuffer_albedo",
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    gbufferNormalHandle_ =
            renderGraph_->createTransientImage("gbuffer_normal",
                                               VK_FORMAT_R16G16B16A16_SFLOAT,
                                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    gbufferDepthHandle_ = renderGraph_->createTransientImage("gbuffer_depth",
                                                             VK_FORMAT_D32_SFLOAT,
                                                             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                                     VK_IMAGE_USAGE_SAMPLED_BIT);

    // --- Geometry pass ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "GeometryPass";
        n.writes = {
                {gbufferAlbedoHandle_,
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                 VK_IMAGE_ASPECT_COLOR_BIT},
                {gbufferNormalHandle_,
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                 VK_IMAGE_ASPECT_COLOR_BIT},
                {gbufferDepthHandle_,
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                 VK_IMAGE_ASPECT_DEPTH_BIT},
        };
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            const VkRect2D fullViewport{{0, 0}, extent};
            const VkRect2D* viewportOverride = ctx.isOffscreen ? &fullViewport : nullptr;
            geometryPass_.record(cmd,
                                 currentFrameGeometryDescriptorSet_,
                                 *currentFrameGeometryBuffer_,
                                 graph.getImageView(gbufferAlbedoHandle_),
                                 graph.getImageView(gbufferNormalHandle_),
                                 graph.getImageView(gbufferDepthHandle_),
                                 extent,
                                 ctx.camera,
                                 ctx.cameraTransform,
                                 ctx.drawQueue,
                                 viewportOverride);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- N lighting passes (one per light slot, additive after the first)
    for (uint32_t s = 0; s < MAX_SHADOW_LIGHTS; ++s) {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "LightingPass_" + std::to_string(s);
        n.reads = {{gbufferAlbedoHandle_,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT},
                   {gbufferNormalHandle_,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT},
                   // The shadow resource is a cubemap; the full cube view is used for sampling.
                   // For directional/spot only face 0 was written, but sampling an uninitialized
                   // face returns 1.0 (farthest depth), which correctly means "no shadow".
                   {shadowDepthHandles_[s],
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT},
                   {gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this, s](VkCommandBuffer cmd,
                              const VulkanRenderGraph<EditorRenderData>& graph,
                              const EditorRenderData& ctx) -> bool {
            const bool isFirst = (s == 0);
            // Slot 0 always runs to clear/write ambient; others skip if no light for that slot.
            if (!isFirst && s >= ctx.lightsWithTransforms.size()) return false;

            if (isFirst) lightingPass_.updateLights(ctx.lightsWithTransforms);
            lightingPass_.uploadPassHeader(s, isFirst);

            const size_t frameIdx = frameManager_.currentFrameIndex();
            VkDescriptorSet ds = VK_NULL_HANDLE;
            const uint32_t handleIdx = ctx.camera.renderTarget.index;
            if (ctx.camera.renderTarget.isValid()) {
                auto lit = renderTargetLightingDescriptorSets_.find(handleIdx);
                if (lit != renderTargetLightingDescriptorSets_.end() && frameIdx < lit->second.size())
                    ds = lit->second[frameIdx][s];
            } else {
                ds = mainLightingDescriptorSets_[frameIdx][s];
            }

            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            const float gbW = static_cast<float>(graph.getWidth(gbufferAlbedoHandle_));
            const float gbH = static_cast<float>(graph.getHeight(gbufferAlbedoHandle_));
            int fbX = 0, fbY = 0, fbW = 0, fbH = 0;
            if (ctx.isOffscreen) {
                fbW = static_cast<int>(extent.width);
                fbH = static_cast<int>(extent.height);
            } else {
                const SceneViewport sv = window_.getSceneViewport();
                const auto [scaleX, scaleY] = window_.getContentScale();
                fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
                fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
                fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
                fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);
            }
            // shadowDepthHandles_[s] is a cubemap. For directional/spot, binding 3 (shadowMap)
            // receives the cube's full view — the 2D shadow path only accesses face 0 UVs so
            // sampling a cube view with a 2D sampler would be invalid. Instead we pass face 0's
            // 2D view for the sampler2D binding and the cube view for the samplerCube binding.
            // The shader selects which to use based on the light type.
            VkImageView face0View = graph.getFaceView(shadowDepthHandles_[s], 0);
            VkImageView cubeView = graph.getImageView(shadowDepthHandles_[s]);
            VkSampler shadowSampler = graph.getSampler(shadowDepthHandles_[s]);

            lightingPass_.record(cmd,
                                 ds,
                                 graph.getImageView(colorTargetHandle_),
                                 extent,
                                 graph.getImageView(gbufferAlbedoHandle_),
                                 graph.getSampler(gbufferAlbedoHandle_),
                                 graph.getImageView(gbufferNormalHandle_),
                                 graph.getSampler(gbufferNormalHandle_),
                                 face0View,
                                 shadowSampler,
                                 cubeView,
                                 shadowSampler,
                                 graph.getImageView(gbufferDepthHandle_),
                                 graph.getSampler(gbufferDepthHandle_),
                                 shadowPass_->lightSpaceMatrix(s),
                                 ctx.camera,
                                 ctx.cameraTransform,
                                 fbX,
                                 fbY,
                                 fbW,
                                 fbH,
                                 gbW,
                                 gbH,
                                 settings_.enableLighting,
                                 isFirst);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- Sky pass (after lighting so LOAD_OP_CLEAR doesn't wipe it; depth test at 1.0 fills sky pixels) ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "SkyPass";
        n.reads = {{gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            int fbX = 0, fbY = 0, fbW = 0, fbH = 0;
            if (ctx.isOffscreen) {
                fbW = static_cast<int>(extent.width);
                fbH = static_cast<int>(extent.height);
            } else {
                const SceneViewport sv = window_.getSceneViewport();
                const auto [scaleX, scaleY] = window_.getContentScale();
                fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
                fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
                fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
                fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);
            }
            skyPass_->record(cmd,
                             graph.getImageView(colorTargetHandle_),
                             graph.getImageView(gbufferDepthHandle_),
                             extent,
                             ctx.camera,
                             ctx.cameraTransform,
                             fbX,
                             fbY,
                             fbW,
                             fbH);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- Particle pass ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "ParticlePass";
        n.reads = {{gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (ctx.particleEmitters.empty()) return true;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            const uint32_t frameIdx = static_cast<uint32_t>(frameManager_.currentFrameIndex());
            particlePass_->record(cmd,
                                  ctx.particleEmitters,
                                  frameIdx,
                                  deltaTime_,
                                  graph.getImageView(colorTargetHandle_),
                                  graph.getImageView(gbufferDepthHandle_),
                                  extent,
                                  currentFrameGeometryDescriptorSet_,
                                  ctx.isOffscreen ? nullptr : &window_);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- Outline pass ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "OutlinePass";
        n.reads = {{objectIdColorHandle_,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT}};
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (ctx.isOffscreen) return false;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            outlinePass_.record(cmd,
                                currentFrameOutlineDescriptorSet_,
                                graph.getImageView(colorTargetHandle_),
                                extent,
                                graph.getImageView(objectIdColorHandle_),
                                graph.getSampler(objectIdColorHandle_),
                                window_,
                                objectIdMap_,
                                ctx.outlineQueue,
                                extent.width,
                                extent.height);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- Grid pass ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "GridPass";
        n.reads = {{gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (!settings_.enableGrid || !ctx.drawGrid) return false;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            gridPass_.record(cmd,
                             graph.getImageView(colorTargetHandle_),
                             extent,
                             ctx.camera,
                             ctx.cameraTransform,
                             ctx.gridScale,
                             graph.getImageView(gbufferDepthHandle_),
                             window_);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- Gizmo pass ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "GizmoPass";
        n.reads = {{gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (ctx.isOffscreen) return false;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            gizmoPass_.record(cmd,
                              graph.getImageView(colorTargetHandle_),
                              extent,
                              ctx.camera,
                              ctx.cameraTransform,
                              window_,
                              graph.getImageView(gbufferDepthHandle_),
                              ctx.gizmoLines);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- Gizmo overlay pass (depth-ignored, for transform handles) ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "GizmoOverlayPass";
        n.reads = {{gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (ctx.isOffscreen) return false;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            gizmoOverlayPass_.record(cmd,
                                     graph.getImageView(colorTargetHandle_),
                                     extent,
                                     ctx.camera,
                                     ctx.cameraTransform,
                                     window_,
                                     graph.getImageView(gbufferDepthHandle_),
                                     ctx.overlayGizmoLines);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- Text pass ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "TextPass";
        n.reads = {{gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (ctx.textQueue.empty()) return false;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            textPass_->record(cmd,
                              graph.getImageView(colorTargetHandle_),
                              extent,
                              ctx.camera,
                              ctx.cameraTransform,
                              window_,
                              ctx.textQueue,
                              graph.getImageView(gbufferDepthHandle_));
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    // --- UI pass (main camera only) ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "UIPass";
        n.writes = {{colorTargetHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (ctx.isOffscreen) return false;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            uiPass_.record(cmd, graph.getImageView(colorTargetHandle_), extent);
            return true;
        };
        renderGraph_->addPass(std::move(n));
    }

    renderGraph_->build();
}
