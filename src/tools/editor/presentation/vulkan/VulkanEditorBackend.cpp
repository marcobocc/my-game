#include "VulkanEditorBackend.hpp"
#include <imgui_impl_vulkan.h>
#include <volk.h>
#include "../../../../engine/data/RenderTargetHandle.hpp"
#include "../../../../engine/systems/core/GameWindow.hpp"
#include "../../../../engine/systems/rendering/vulkan/core/VulkanFrameManager.hpp"
#include "../../../../engine/systems/rendering/vulkan/core/VulkanSwapchainManager.hpp"
#include "../../../../engine/systems/rendering/vulkan/core/resources/VulkanRenderTargetManager.hpp"
#include "../../../../engine/systems/rendering/vulkan/passes/VulkanGeometryPass.hpp"
#include "../../../../engine/systems/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../../engine/systems/rendering/vulkan/passes/VulkanGridPass.hpp"
#include "../../../../engine/systems/rendering/vulkan/passes/VulkanLightingPass.hpp"
#include "../../../../engine/systems/rendering/vulkan/passes/VulkanObjectIdPass.hpp"
#include "../../../../engine/systems/rendering/vulkan/passes/VulkanOutlinePass.hpp"
#include "../../../../engine/systems/rendering/vulkan/passes/VulkanUIPass.hpp"

VulkanEditorBackend::VulkanEditorBackend(GameWindow& window,
                                         VulkanContext& context,
                                         VulkanFrameManager& frameManager,
                                         VulkanRenderTargetManager& renderTargetManager,
                                         VulkanGeometryPass& geometryPass,
                                         VulkanLightingPass& lightingPass,
                                         VulkanGridPass& gridPass,
                                         VulkanGizmoPass& gizmoPass,
                                         VulkanObjectIdPass& objectIdPass,
                                         VulkanOutlinePass& outlinePass,
                                         VulkanUIPass& uiPass,
                                         VulkanSwapchainManager& swapchainManager,
                                         RendererSettings& settings) :
    window_(window),
    context_(context),
    settings_(settings),
    swapchainManager_(swapchainManager),
    frameManager_(frameManager),
    renderTargetManager_(renderTargetManager),
    geometryPass_(geometryPass),
    lightingPass_(lightingPass),
    gridPass_(gridPass),
    gizmoPass_(gizmoPass),
    objectIdPass_(objectIdPass),
    outlinePass_(outlinePass),
    uiPass_(uiPass),
    frameCount_(static_cast<uint32_t>(swapchainManager.swapchain().swapchainImages.size())),
    swapchainColorFormat_(swapchainManager.swapchain().swapchainImageFormat) {
    const auto& sc = swapchainManager_.swapchain();
    setupRenderGraph(sc.swapchainImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, sc.swapchainExtent);

    // Allocate per-frame resources for main swapchain render
    mainLightingDescriptorSets_.resize(frameCount_);
    mainOutlineDescriptorSets_.resize(frameCount_);
    mainGeometryDescriptorSets_.resize(frameCount_);
    mainGeometryBuffers_.resize(frameCount_);
    for (uint32_t i = 0; i < frameCount_; ++i) {
        auto [buf, gds] = geometryPass_.createRenderContext();
        mainGeometryBuffers_[i] = std::move(buf);
        mainGeometryDescriptorSets_[i] = gds;
        mainLightingDescriptorSets_[i] = lightingPass_.createDescriptorSet();
        mainOutlineDescriptorSets_[i] = outlinePass_.createDescriptorSet();
    }

    window_.onWindowResize([this](int, int, int, int) {
        swapchainManager_.recreate(context_);
        const auto& extent = swapchainManager_.swapchain().swapchainExtent;
        renderGraph_->onResize(extent.width, extent.height);
    });
}

VulkanEditorBackend::~VulkanEditorBackend() {
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

bool VulkanEditorBackend::renderFrame(const EditorRenderData& renderData) {
    // Off-screen cameras: queue up work to be batched into the next swapchain frame.
    if (renderData.camera.renderTarget.isValid()) {
        if (renderTargetManager_.get(renderData.camera.renderTarget))
            pendingOffscreenCameras_.push_back({renderData.camera, renderData.cameraTransform});
        return true;
    }

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

RenderTargetHandle VulkanEditorBackend::createRenderTarget(uint32_t width, uint32_t height) {
    RenderTargetHandle handle = renderTargetManager_.create(width, height, swapchainColorFormat_);
    if (!handle.isValid()) return handle;

    auto* rt = renderTargetManager_.get(handle);
    if (!rt) return handle;

    // Allocate per-frame resources for this render target
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
        lightingDs[i] = lightingPass_.createDescriptorSet();
        outlineDs[i] = outlinePass_.createDescriptorSet();
    }

    // Register ImGui descriptor set for UI preview
    VkDescriptorSet imguiDescriptorSet =
            ImGui_ImplVulkan_AddTexture(rt->colorSampler, rt->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    renderTargetImGuiSets_[handle.index] = imguiDescriptorSet;
    return handle;
}

void VulkanEditorBackend::destroyRenderTarget(RenderTargetHandle handle) {
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

VkDescriptorSet VulkanEditorBackend::getRenderTargetImGuiId(RenderTargetHandle handle) const {
    auto it = renderTargetImGuiSets_.find(handle.index);
    return (it != renderTargetImGuiSets_.end()) ? it->second : VK_NULL_HANDLE;
}

VulkanRenderTarget VulkanEditorBackend::getRenderTarget(RenderTargetHandle handle) const {
    const auto* rt = renderTargetManager_.get(handle);
    if (!rt) {
        return {0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_FORMAT_UNDEFINED};
    }
    return *rt;
}

// ------------------- Frame helpers -------------------

void VulkanEditorBackend::executeRenderGraph(VkCommandBuffer cmd,
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

void VulkanEditorBackend::recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const EditorRenderData& renderData) {
    // Flush off-screen cameras first, within the same command buffer.
    for (auto& job: pendingOffscreenCameras_) {
        if (job.camera.renderTarget.isValid()) {
            const auto* target = renderTargetManager_.get(job.camera.renderTarget);
            if (target && target->image != VK_NULL_HANDLE) {
                size_t frameIdx = frameManager_.currentFrameIndex();
                uint32_t handleIdx = job.camera.renderTarget.index;

                auto bit = renderTargetGeometryBuffers_.find(handleIdx);
                if (bit != renderTargetGeometryBuffers_.end()) {
                    const auto& geomBufs = bit->second;
                    currentFrameGeometryBuffer_ = (frameIdx < geomBufs.size())
                                                          ? const_cast<VulkanBuffer*>(&geomBufs[frameIdx])
                                                          : const_cast<VulkanBuffer*>(&geomBufs[0]);

                    // Set current frame descriptor sets for render passes
                    auto lit = renderTargetLightingDescriptorSets_.find(handleIdx);
                    auto oit = renderTargetOutlineDescriptorSets_.find(handleIdx);
                    auto git = renderTargetGeometryDescriptorSets_.find(handleIdx);
                    if (lit != renderTargetLightingDescriptorSets_.end()) {
                        currentFrameLightingDescriptorSet_ =
                                (frameIdx < lit->second.size()) ? lit->second[frameIdx] : lit->second[0];
                    }
                    if (oit != renderTargetOutlineDescriptorSets_.end()) {
                        currentFrameOutlineDescriptorSet_ =
                                (frameIdx < oit->second.size()) ? oit->second[frameIdx] : oit->second[0];
                    }
                    if (git != renderTargetGeometryDescriptorSets_.end()) {
                        currentFrameGeometryDescriptorSet_ =
                                (frameIdx < git->second.size()) ? git->second[frameIdx] : git->second[0];
                    }

                    executeRenderGraph(cmd,
                                       target->image,
                                       target->view,
                                       {target->width, target->height},
                                       EditorRenderData{job.camera,
                                                        job.cameraTransform,
                                                        renderData.drawQueue,
                                                        renderData.outlineQueue,
                                                        renderData.gizmoLines,
                                                        /*isOffscreen=*/true});
                    transitionColorImageFinal(cmd,
                                              target->image,
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                              VK_ACCESS_SHADER_READ_BIT);
                }
            }
        }
    }
    pendingOffscreenCameras_.clear();

    VulkanRenderTarget swapchainTarget = swapchainManager_.getSwapchainTarget(imageIndex);
    size_t frameIdx = frameManager_.currentFrameIndex();

    // Set current frame descriptor sets and buffer for render passes
    currentFrameLightingDescriptorSet_ = mainLightingDescriptorSets_[frameIdx];
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

// ------------------- Render graph setup -------------------

void VulkanEditorBackend::setupRenderGraph(VkFormat colorFormat, VkImageUsageFlags colorUsage, VkExtent2D extent) {
    renderGraph_.emplace(context_, extent);

    colorTargetHandle_ = renderGraph_->importImage("swapchain_color", colorFormat, colorUsage);

    objectIdColorHandle_ = renderGraph_->createTransientImage(
            "objectid_color",
            VK_FORMAT_R32_UINT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    objectIdDepthHandle_ = renderGraph_->createTransientImage(
            "objectid_depth", VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

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
                {objectIdDepthHandle_,
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                 VK_IMAGE_ASPECT_DEPTH_BIT},
        };
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<EditorRenderData>& graph,
                           const EditorRenderData& ctx) -> bool {
            if (ctx.isOffscreen) return false;
            objectIdMap_ = objectIdPass_.record(cmd,
                                                ctx.drawQueue,
                                                ctx.camera,
                                                ctx.cameraTransform,
                                                window_,
                                                graph.getImage(objectIdColorHandle_),
                                                graph.getImageView(objectIdColorHandle_),
                                                graph.getImageView(objectIdDepthHandle_),
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
    gbufferDepthHandle_ = renderGraph_->createTransientImage(
            "gbuffer_depth", VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

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

    // --- Lighting pass ---
    {
        VulkanRenderGraph<EditorRenderData>::RenderPassNode n;
        n.name = "LightingPass";
        n.reads = {{gbufferAlbedoHandle_,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT},
                   {gbufferNormalHandle_,
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
            lightingPass_.record(cmd,
                                 currentFrameLightingDescriptorSet_,
                                 graph.getImageView(colorTargetHandle_),
                                 extent,
                                 graph.getImageView(gbufferAlbedoHandle_),
                                 graph.getSampler(gbufferAlbedoHandle_),
                                 graph.getImageView(gbufferNormalHandle_),
                                 graph.getSampler(gbufferNormalHandle_),
                                 fbX,
                                 fbY,
                                 fbW,
                                 fbH,
                                 gbW,
                                 gbH,
                                 settings_.enableLighting);
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
            if (!settings_.enableGrid) return false;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            gridPass_.record(cmd,
                             graph.getImageView(colorTargetHandle_),
                             extent,
                             ctx.camera,
                             ctx.cameraTransform,
                             ctx.gridScale,
                             graph.getImageView(gbufferDepthHandle_));
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
