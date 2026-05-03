#include "VulkanRenderingOrchestrator.hpp"
#include <glm/vec3.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <volk.h>
#include "../../core/GameWindow.hpp"
#include "VulkanCommandManager.hpp"
#include "VulkanPickingBackend.hpp"
#include "VulkanSwapchainManager.hpp"
#include "passes/VulkanGeometryPass.hpp"
#include "passes/VulkanGizmoPass.hpp"
#include "passes/VulkanGridPass.hpp"
#include "passes/VulkanLightingPass.hpp"
#include "passes/VulkanObjectIdPass.hpp"
#include "passes/VulkanOutlinePass.hpp"
#include "passes/VulkanUIPass.hpp"
#include "rendergraph/RenderGraph.hpp"
#include "rendergraph/RenderPassNode.hpp"
#include "utils/error_handling.hpp"
#include "utils/memory.hpp"

VulkanRenderingOrchestrator::VulkanRenderingOrchestrator(GameWindow& window,
                                                         VulkanContext& context,
                                                         VulkanGeometryPass& geometryPass,
                                                         VulkanLightingPass& lightingPass,
                                                         VulkanGridPass& gridPass,
                                                         VulkanGizmoPass& gizmoPass,
                                                         VulkanObjectIdPass& objectIdPass,
                                                         VulkanPickingBackend& pickingBackend,
                                                         VulkanOutlinePass& outlinePass,
                                                         VulkanUIPass& uiPass,
                                                         VulkanCommandManager& commandManager,
                                                         VulkanSwapchainManager& swapchainManager,
                                                         RendererSettings& settings) :
    window_(window),
    context_(context),
    geometryPass_(geometryPass),
    lightingPass_(lightingPass),
    gridPass_(gridPass),
    gizmoPass_(gizmoPass),
    objectIdPass_(objectIdPass),
    pickingBackend_(pickingBackend),
    outlinePass_(outlinePass),
    uiPass_(uiPass),
    commandManager_(commandManager),
    swapchainManager_(swapchainManager),
    settings_(settings),
    graph_(context, swapchainManager) {
    initFrameSync();
    setupGraph();
    window_.onWindowResize([this](int, int, int, int) {
        swapchainManager_.recreate(context_);
        const auto& extent = swapchainManager_.swapchain().swapchainExtent;
        graph_.onResize(extent.width, extent.height);
        outlinePass_.resize(extent.width, extent.height);
    });
}

VulkanRenderingOrchestrator::~VulkanRenderingOrchestrator() {
    vkDeviceWaitIdle(context_.device);
    for (auto& rt: renderTargets_) {
        if (rt) freeRenderTargetImages(*rt);
    }
    renderTargets_.clear();
    destroyFrameSync();
}

void VulkanRenderingOrchestrator::enqueueForDrawing(const Renderer& renderer,
                                                    const Transform& transform,
                                                    std::string objectId) {
    drawQueue_.push_back({renderer, transform, std::move(objectId)});
}

void VulkanRenderingOrchestrator::enqueueForOutline(const Renderer& renderer,
                                                    const Transform& transform,
                                                    std::string objectId) const {
    outlinePass_.enqueueForOutline(renderer, transform, std::move(objectId));
}

void VulkanRenderingOrchestrator::submitGizmoLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) const {
    gizmoPass_.submitLine(from, to, color);
}

bool VulkanRenderingOrchestrator::renderFrame(const Camera& camera, const Transform& cameraTransform) {
    waitForCurrentFrame();
    pickingBackend_.processReadback(objectIdPass_.getObjectIdMap());

    uint32_t imageIndex = 0;
    if (!acquireImage(imageIndex)) return false;
    VkCommandBuffer cmd = beginFrame();
    recordCommands(cmd, imageIndex, camera, cameraTransform);
    endFrame(cmd, imageIndex);
    advanceFrame();
    return true;
}

// ------------------- Graph setup -------------------

void VulkanRenderingOrchestrator::setupGraph() {
    const VulkanSwapchain& sc = swapchainManager_.swapchain();

    swapchainColorHandle_ =
            graph_.importImage("swapchain_color", sc.swapchainImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    objectIdColorHandle_ = graph_.createTransientImage(
            "objectid_color",
            VK_FORMAT_R32_UINT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    objectIdDepthHandle_ = graph_.createTransientImage(
            "objectid_depth", VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // --- ObjectId pass ---
    {
        RenderPassNode n;
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
        // The pass itself transitions objectId color to TRANSFER_SRC at the end of record().
        n.epilogues = {{objectIdColorHandle_,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT}};
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph& graph) {
            objectIdPass_.record(cmd,
                                 drawQueue_,
                                 *currentCamera_,
                                 *currentCameraTransform_,
                                 window_,
                                 graph.getImage(objectIdColorHandle_),
                                 graph.getImageView(objectIdColorHandle_),
                                 graph.getImageView(objectIdDepthHandle_),
                                 graph.getWidth(objectIdColorHandle_),
                                 graph.getHeight(objectIdColorHandle_));
        };
        n.postExecute = [this](VkCommandBuffer cmd, const RenderGraph& graph) {
            pickingBackend_.recordReadback(cmd,
                                           graph.getImage(objectIdColorHandle_),
                                           graph.getWidth(objectIdColorHandle_),
                                           graph.getHeight(objectIdColorHandle_));
        };
        graph_.addPass(std::move(n));
    }

    gbufferAlbedoHandle_ =
            graph_.createTransientImage("gbuffer_albedo",
                                        VK_FORMAT_R8G8B8A8_UNORM,
                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    gbufferNormalHandle_ =
            graph_.createTransientImage("gbuffer_normal",
                                        VK_FORMAT_R16G16B16A16_SFLOAT,
                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    gbufferDepthHandle_ = graph_.createTransientImage(
            "gbuffer_depth", VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // --- Geometry pass (G-buffer) ---
    {
        RenderPassNode n;
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
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph& graph) {
            const VkExtent2D extent = swapchainManager_.swapchain().swapchainExtent;
            geometryPass_.record(cmd,
                                 graph.getImageView(gbufferAlbedoHandle_),
                                 graph.getImageView(gbufferNormalHandle_),
                                 graph.getImageView(gbufferDepthHandle_),
                                 extent,
                                 *currentCamera_,
                                 *currentCameraTransform_,
                                 drawQueue_);
        };
        graph_.addPass(std::move(n));
    }

    // --- Lighting pass ---
    {
        RenderPassNode n;
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
        n.writes = {{swapchainColorHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph& graph) {
            const VkExtent2D extent = swapchainManager_.swapchain().swapchainExtent;
            const SceneViewport sv = window_.getSceneViewport();
            const auto [scaleX, scaleY] = window_.getContentScale();
            const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
            const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
            const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
            const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);
            lightingPass_.record(cmd,
                                 currentImageIndex_,
                                 graph.getImageView(swapchainColorHandle_),
                                 extent,
                                 graph.getImageView(gbufferAlbedoHandle_),
                                 graph.getSampler(gbufferAlbedoHandle_),
                                 graph.getImageView(gbufferNormalHandle_),
                                 graph.getSampler(gbufferNormalHandle_),
                                 fbX,
                                 fbY,
                                 fbW,
                                 fbH,
                                 settings_.enableLighting);
        };
        graph_.addPass(std::move(n));
    }

    // --- Outline pass ---
    {
        RenderPassNode n;
        n.name = "OutlinePass";
        n.reads = {{objectIdColorHandle_,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT}};
        n.writes = {{swapchainColorHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph& graph) {
            outlinePass_.record(cmd,
                                currentImageIndex_,
                                swapchainManager_.swapchain(),
                                graph.getImageView(objectIdColorHandle_),
                                graph.getSampler(objectIdColorHandle_),
                                window_,
                                objectIdPass_.getObjectIdMap());
        };
        graph_.addPass(std::move(n));
    }

    // --- Grid pass ---
    {
        RenderPassNode n;
        n.name = "GridPass";
        n.reads = {{gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{swapchainColorHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph& graph) {
            if (!settings_.enableGrid) return;
            gridPass_.record(cmd,
                             currentImageIndex_,
                             *currentCamera_,
                             *currentCameraTransform_,
                             graph.getImageView(gbufferDepthHandle_));
        };
        graph_.addPass(std::move(n));
    }

    // --- Gizmo pass ---
    {
        RenderPassNode n;
        n.name = "GizmoPass";
        n.reads = {{gbufferDepthHandle_,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.writes = {{swapchainColorHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph& graph) {
            gizmoPass_.record(cmd,
                              currentImageIndex_,
                              *currentCamera_,
                              *currentCameraTransform_,
                              window_,
                              graph.getImageView(gbufferDepthHandle_));
        };
        graph_.addPass(std::move(n));
    }

    // --- UI pass ---
    {
        RenderPassNode n;
        n.name = "UIPass";
        n.writes = {{swapchainColorHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph&) { uiPass_.record(cmd, currentImageIndex_); };
        graph_.addPass(std::move(n));
    }

    // --- Present node: transitions swapchain color to PRESENT_SRC_KHR ---
    {
        RenderPassNode n;
        n.name = "Present";
        n.reads = {{swapchainColorHandle_,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    0,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT}};
        graph_.addPass(std::move(n));
    }

    graph_.build();
}

// ------------------- Frame sync -------------------

void VulkanRenderingOrchestrator::initFrameSync() {
    const size_t frameCount = swapchainManager_.swapchain().swapchainImages.size();
    frames_.resize(frameCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& frame: frames_) {
        throwIfUnsuccessful(vkCreateFence(context_.device, &fenceInfo, nullptr, &frame.inFlightFence));
        throwIfUnsuccessful(vkCreateSemaphore(context_.device, &semaphoreInfo, nullptr, &frame.imageAvailable));
        throwIfUnsuccessful(vkCreateSemaphore(context_.device, &semaphoreInfo, nullptr, &frame.renderFinished));
    }
}

void VulkanRenderingOrchestrator::destroyFrameSync() {
    for (auto& frame: frames_) {
        if (frame.inFlightFence != VK_NULL_HANDLE) {
            vkWaitForFences(context_.device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(context_.device, frame.inFlightFence, nullptr);
            frame.inFlightFence = VK_NULL_HANDLE;
        }
        if (frame.imageAvailable != VK_NULL_HANDLE) {
            vkDestroySemaphore(context_.device, frame.imageAvailable, nullptr);
            frame.imageAvailable = VK_NULL_HANDLE;
        }
        if (frame.renderFinished != VK_NULL_HANDLE) {
            vkDestroySemaphore(context_.device, frame.renderFinished, nullptr);
            frame.renderFinished = VK_NULL_HANDLE;
        }
    }
}

void VulkanRenderingOrchestrator::waitForCurrentFrame() {
    VkFence fence = currentFrame().inFlightFence;
    if (fence != VK_NULL_HANDLE) {
        vkWaitForFences(context_.device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(context_.device, 1, &fence);
    }
}

void VulkanRenderingOrchestrator::advanceFrame() { currentFrameIndex_ = (currentFrameIndex_ + 1) % frames_.size(); }

VulkanRenderingOrchestrator::FrameSync& VulkanRenderingOrchestrator::currentFrame() {
    return frames_.at(currentFrameIndex_);
}

// ------------------- Frame stages -------------------

bool VulkanRenderingOrchestrator::acquireImage(uint32_t& imageIndex) {
    if (!swapchainManager_.acquireNextImage(context_, currentFrame().imageAvailable, imageIndex)) return false;
    if (imageIndex >= swapchainManager_.swapchain().swapchainImages.size()) return false;
    return true;
}

VkCommandBuffer VulkanRenderingOrchestrator::beginFrame() const {
    commandManager_.beginFrame();
    VkCommandBuffer cmd = commandManager_.allocateCommandBuffer();
    VulkanCommandManager::beginCommandBuffer(cmd);
    return cmd;
}

void VulkanRenderingOrchestrator::recordCommands(VkCommandBuffer cmd,
                                                 uint32_t imageIndex,
                                                 const Camera& camera,
                                                 const Transform& cameraTransform) {
    currentImageIndex_ = imageIndex;

    executePendingRenderTargetJobs(cmd);

    currentCamera_ = &camera;
    currentCameraTransform_ = &cameraTransform;

    const VulkanSwapchain& sc = swapchainManager_.swapchain();
    graph_.setImportedImage(swapchainColorHandle_, sc.swapchainImages[imageIndex], sc.swapchainImageViews[imageIndex]);
    graph_.resetLayout(swapchainColorHandle_);

    graph_.execute(cmd);
}

void VulkanRenderingOrchestrator::endFrame(VkCommandBuffer cmd, uint32_t imageIndex) {
    VulkanCommandManager::endCommandBuffer(cmd);
    submit(cmd, imageIndex);
    commandManager_.endFrame();
    drawQueue_.clear();
}

void VulkanRenderingOrchestrator::submit(VkCommandBuffer cmd, uint32_t imageIndex) {
    const auto& frame = currentFrame();
    commandManager_.submitCommandBuffer(cmd, frame.imageAvailable, frame.renderFinished, frame.inFlightFence);
    swapchainManager_.present(context_.graphicsQueue, frame.renderFinished, imageIndex);
}

// ------------------- Render targets -------------------

RenderTargetHandle VulkanRenderingOrchestrator::createRenderTarget(uint32_t width, uint32_t height) {
    RenderTargetHandle handle{static_cast<uint32_t>(renderTargets_.size())};
    auto rt = std::make_unique<RenderTargetData>();
    rt->width = width;
    rt->height = height;
    allocateRenderTargetImages(*rt);
    rt->imguiDescriptorSet =
            ImGui_ImplVulkan_AddTexture(rt->colorSampler, rt->colorView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    renderTargets_.push_back(std::move(rt));
    return handle;
}

void VulkanRenderingOrchestrator::destroyRenderTarget(RenderTargetHandle handle) {
    if (!handle.isValid() || handle.index >= renderTargets_.size()) return;
    auto& rt = renderTargets_[handle.index];
    if (!rt) return;
    vkDeviceWaitIdle(context_.device);
    if (rt->imguiDescriptorSet != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(rt->imguiDescriptorSet);
        rt->imguiDescriptorSet = VK_NULL_HANDLE;
    }
    freeRenderTargetImages(*rt);
    rt.reset();
}

VkDescriptorSet VulkanRenderingOrchestrator::getRenderTargetImGuiId(RenderTargetHandle handle) const {
    if (!handle.isValid() || handle.index >= renderTargets_.size()) return VK_NULL_HANDLE;
    const auto& rt = renderTargets_[handle.index];
    if (!rt) return VK_NULL_HANDLE;
    return rt->imguiDescriptorSet;
}

void VulkanRenderingOrchestrator::renderToTarget(RenderTargetHandle handle,
                                                 const Camera& camera,
                                                 const Transform& cameraTransform,
                                                 const std::vector<DrawCall>& drawQueue) {
    if (!handle.isValid() || handle.index >= renderTargets_.size()) return;
    if (!renderTargets_[handle.index]) return;
    pendingRenderTargetJobs_.push_back({handle, camera, cameraTransform, drawQueue});
}

void VulkanRenderingOrchestrator::renderToTarget(RenderTargetHandle handle,
                                                 const Camera& camera,
                                                 const Transform& cameraTransform) {
    if (!handle.isValid() || handle.index >= renderTargets_.size()) return;
    if (!renderTargets_[handle.index]) return;
    pendingRenderTargetJobs_.push_back({handle, camera, cameraTransform, drawQueue_});
}

void VulkanRenderingOrchestrator::executePendingRenderTargetJobs(VkCommandBuffer cmd) {
    for (auto& job: pendingRenderTargetJobs_) {
        auto& rt = renderTargets_[job.handle.index];
        if (!rt) continue;
        renderCameraToTarget(cmd, *rt, job.camera, job.cameraTransform, job.drawQueue);
    }
    pendingRenderTargetJobs_.clear();
}

void VulkanRenderingOrchestrator::renderCameraToTarget(VkCommandBuffer cmd,
                                                       RenderTargetData& rt,
                                                       const Camera& camera,
                                                       const Transform& cameraTransform,
                                                       const std::vector<DrawCall>& drawQueue) {
    const VkExtent2D extent{rt.width, rt.height};
    const VkRect2D viewport{{0, 0}, extent};

    auto emitBarrier = [&](VkImage image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           VkAccessFlags srcAccess,
                           VkAccessFlags dstAccess,
                           VkPipelineStageFlags srcStage,
                           VkPipelineStageFlags dstStage,
                           VkImageAspectFlags aspect) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {aspect, 0, 1, 0, 1};
        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    };

    // Transition G-buffers to attachment layouts
    emitBarrier(rt.gbufferAlbedo,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                0,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT);
    emitBarrier(rt.gbufferNormal,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                0,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT);
    emitBarrier(rt.gbufferDepth,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT);

    // Geometry pass
    geometryPass_.record(cmd,
                         rt.geometryContext,
                         rt.gbufferAlbedoView,
                         rt.gbufferNormalView,
                         rt.gbufferDepthView,
                         extent,
                         camera,
                         cameraTransform,
                         drawQueue,
                         &viewport);

    // Transition G-buffers to shader read for lighting
    emitBarrier(rt.gbufferAlbedo,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT);
    emitBarrier(rt.gbufferNormal,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT);

    // Transition color target for lighting output
    emitBarrier(rt.colorImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                0,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT);

    // Lighting pass
    lightingPass_.record(cmd,
                         rt.lightingDescSetIndex,
                         rt.colorView,
                         extent,
                         rt.gbufferAlbedoView,
                         rt.gbufferAlbedoSampler,
                         rt.gbufferNormalView,
                         rt.gbufferNormalSampler,
                         0,
                         0,
                         static_cast<int>(rt.width),
                         static_cast<int>(rt.height),
                         true);

    // Transition color to shader read for ImGui sampling
    emitBarrier(rt.colorImage,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanRenderingOrchestrator::allocateRenderTargetImages(RenderTargetData& rt) {
    auto allocImage = [&](VkFormat format, VkImageUsageFlags usage, VkImage& outImage, VkDeviceMemory& outMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {rt.width, rt.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(context_.device, &imageInfo, nullptr, &outImage);

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(context_.device, outImage, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
                getMemoryType(context_.physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(context_.device, &allocInfo, nullptr, &outMemory);
        vkBindImageMemory(context_.device, outImage, outMemory, 0);
    };

    auto allocView = [&](VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageView& outView) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange = {aspect, 0, 1, 0, 1};
        vkCreateImageView(context_.device, &viewInfo, nullptr, &outView);
    };

    auto allocSampler = [&](VkFilter filter, VkSampler& outSampler) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = filter;
        samplerInfo.minFilter = filter;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(context_.device, &samplerInfo, nullptr, &outSampler);
    };

    // Color output
    allocImage(RenderTargetData::COLOR_FORMAT,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               rt.colorImage,
               rt.colorMemory);
    allocView(rt.colorImage, RenderTargetData::COLOR_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, rt.colorView);
    allocSampler(VK_FILTER_LINEAR, rt.colorSampler);

    // Depth
    allocImage(
            RenderTargetData::DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, rt.depthImage, rt.depthMemory);
    allocView(rt.depthImage, RenderTargetData::DEPTH_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT, rt.depthView);

    // G-buffer albedo
    allocImage(RenderTargetData::GBUFFER_ALBEDO_FORMAT,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               rt.gbufferAlbedo,
               rt.gbufferAlbedoMemory);
    allocView(
            rt.gbufferAlbedo, RenderTargetData::GBUFFER_ALBEDO_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, rt.gbufferAlbedoView);
    allocSampler(VK_FILTER_NEAREST, rt.gbufferAlbedoSampler);

    // G-buffer normal
    allocImage(RenderTargetData::GBUFFER_NORMAL_FORMAT,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               rt.gbufferNormal,
               rt.gbufferNormalMemory);
    allocView(
            rt.gbufferNormal, RenderTargetData::GBUFFER_NORMAL_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT, rt.gbufferNormalView);
    allocSampler(VK_FILTER_NEAREST, rt.gbufferNormalSampler);

    // G-buffer depth
    allocImage(RenderTargetData::DEPTH_FORMAT,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               rt.gbufferDepth,
               rt.gbufferDepthMemory);
    allocView(rt.gbufferDepth, RenderTargetData::DEPTH_FORMAT, VK_IMAGE_ASPECT_DEPTH_BIT, rt.gbufferDepthView);

    // Per-target rendering resources
    rt.geometryContext = geometryPass_.createRenderContext();
    rt.lightingDescSetIndex = lightingPass_.allocateExtraDescriptorSet();
}

void VulkanRenderingOrchestrator::freeRenderTargetImages(RenderTargetData& rt) {
    geometryPass_.destroyRenderContext(rt.geometryContext);

    auto freeImage = [&](VkImage& image, VkDeviceMemory& memory) {
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(context_.device, image, nullptr);
            image = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(context_.device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    };
    auto freeView = [&](VkImageView& view) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(context_.device, view, nullptr);
            view = VK_NULL_HANDLE;
        }
    };
    auto freeSampler = [&](VkSampler& sampler) {
        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(context_.device, sampler, nullptr);
            sampler = VK_NULL_HANDLE;
        }
    };

    freeSampler(rt.colorSampler);
    freeView(rt.colorView);
    freeImage(rt.colorImage, rt.colorMemory);

    freeView(rt.depthView);
    freeImage(rt.depthImage, rt.depthMemory);

    freeSampler(rt.gbufferAlbedoSampler);
    freeView(rt.gbufferAlbedoView);
    freeImage(rt.gbufferAlbedo, rt.gbufferAlbedoMemory);

    freeSampler(rt.gbufferNormalSampler);
    freeView(rt.gbufferNormalView);
    freeImage(rt.gbufferNormal, rt.gbufferNormalMemory);

    freeView(rt.gbufferDepthView);
    freeImage(rt.gbufferDepth, rt.gbufferDepthMemory);
}
