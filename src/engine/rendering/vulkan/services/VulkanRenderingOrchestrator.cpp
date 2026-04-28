#include "VulkanRenderingOrchestrator.hpp"
#include <glm/vec3.hpp>
#include <volk.h>
#include "VulkanCommandManager.hpp"
#include "VulkanPickingBackend.hpp"
#include "VulkanSwapchainManager.hpp"
#include "core/GameWindow.hpp"
#include "passes/VulkanGizmoPass.hpp"
#include "passes/VulkanGridPass.hpp"
#include "passes/VulkanObjectIdPass.hpp"
#include "passes/VulkanOutlinePass.hpp"
#include "passes/VulkanScenePass.hpp"
#include "passes/VulkanUIPass.hpp"
#include "rendering/vulkan/core/error_handling.hpp"
#include "rendering/vulkan/rendergraph/RenderGraph.hpp"
#include "rendering/vulkan/rendergraph/RenderPassNode.hpp"

VulkanRenderingOrchestrator::VulkanRenderingOrchestrator(GameWindow& window,
                                                         VulkanContext& context,
                                                         VulkanScenePass& scenePass,
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
    scenePass_(scenePass),
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
    destroyFrameSync();
}

void VulkanRenderingOrchestrator::enqueueForDrawing(const Renderer& renderer,
                                                    const Transform& transform,
                                                    std::string objectId) const {
    scenePass_.enqueueForDrawing(renderer, transform, std::move(objectId));
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

    swapchainDepthHandle_ =
            graph_.importImage("swapchain_depth", sc.depthBuffer.format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

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
                                 scenePass_.getDrawQueue(),
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

    // --- Scene pass ---
    {
        RenderPassNode n;
        n.name = "ScenePass";
        n.writes = {
                {swapchainColorHandle_,
                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                 VK_IMAGE_ASPECT_COLOR_BIT},
                {swapchainDepthHandle_,
                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                 VK_IMAGE_ASPECT_DEPTH_BIT},
        };
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph& graph) {
            const VkExtent2D extent = swapchainManager_.swapchain().swapchainExtent;
            scenePass_.record(cmd,
                              graph.getImageView(swapchainColorHandle_),
                              graph.getImageView(swapchainDepthHandle_),
                              extent,
                              *currentCamera_,
                              *currentCameraTransform_);
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
        // Swapchain color is already COLOR_ATTACHMENT from ScenePass — no new write entry needed,
        // but we must declare it so the graph knows this pass touches it.
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
                                window_);
        };
        graph_.addPass(std::move(n));
    }

    // --- Grid pass ---
    {
        RenderPassNode n;
        n.name = "GridPass";
        n.writes = {{swapchainColorHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph&) {
            if (!settings_.enableGrid) return;
            gridPass_.record(cmd, currentImageIndex_, *currentCamera_, *currentCameraTransform_);
        };
        graph_.addPass(std::move(n));
    }

    // --- Gizmo pass ---
    {
        RenderPassNode n;
        n.name = "GizmoPass";
        n.writes = {{swapchainColorHandle_,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_ASPECT_COLOR_BIT}};
        n.execute = [this](VkCommandBuffer cmd, const RenderGraph&) {
            gizmoPass_.record(cmd, currentImageIndex_, *currentCamera_, *currentCameraTransform_, window_);
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
    currentCamera_ = &camera;
    currentCameraTransform_ = &cameraTransform;
    currentImageIndex_ = imageIndex;

    const VulkanSwapchain& sc = swapchainManager_.swapchain();
    graph_.setImportedImage(swapchainColorHandle_, sc.swapchainImages[imageIndex], sc.swapchainImageViews[imageIndex]);
    graph_.setImportedImage(swapchainDepthHandle_, sc.depthBuffer.image, sc.depthBuffer.view);
    graph_.resetLayout(swapchainColorHandle_);
    graph_.resetLayout(swapchainDepthHandle_);

    graph_.execute(cmd);
}

void VulkanRenderingOrchestrator::endFrame(VkCommandBuffer cmd, uint32_t imageIndex) {
    VulkanCommandManager::endCommandBuffer(cmd);
    submit(cmd, imageIndex);
    commandManager_.endFrame();
}

void VulkanRenderingOrchestrator::submit(VkCommandBuffer cmd, uint32_t imageIndex) {
    const auto& frame = currentFrame();
    commandManager_.submitCommandBuffer(cmd, frame.imageAvailable, frame.renderFinished, frame.inFlightFence);
    swapchainManager_.present(context_.graphicsQueue, frame.renderFinished, imageIndex);
}
