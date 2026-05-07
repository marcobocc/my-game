#include "VulkanGameRenderer.hpp"
#include <volk.h>
#include "core/VulkanFrameManager.hpp"
#include "core/VulkanSwapchainManager.hpp"
#include "core/resources/VulkanRenderTargetManager.hpp"
#include "passes/VulkanGeometryPass.hpp"
#include "passes/VulkanLightingPass.hpp"

VulkanGameRenderer::VulkanGameRenderer(VulkanContext& context,
                                       VulkanFrameManager& frameManager,
                                       VulkanRenderTargetManager& renderTargetManager,
                                       VulkanGeometryPass& geometryPass,
                                       VulkanLightingPass& lightingPass,
                                       VulkanSwapchainManager& swapchainManager,
                                       RendererSettings& settings) :
    context_(context),
    settings_(settings),
    swapchainManager_(swapchainManager),
    frameManager_(frameManager),
    renderTargetManager_(renderTargetManager),
    geometryPass_(geometryPass),
    lightingPass_(lightingPass),
    frameCount_(static_cast<uint32_t>(swapchainManager.swapchain().swapchainImages.size())),
    swapchainColorFormat_(swapchainManager.swapchain().swapchainImageFormat) {
    const auto& sc = swapchainManager_.swapchain();
    setupRenderGraph(sc.swapchainImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, sc.swapchainExtent);

    // Allocate per-frame resources
    mainLightingDescriptorSets_.resize(frameCount_);
    mainGeometryDescriptorSets_.resize(frameCount_);
    mainGeometryBuffers_.resize(frameCount_);
    for (uint32_t i = 0; i < frameCount_; ++i) {
        auto [buf, gds] = geometryPass_.createRenderContext();
        mainGeometryBuffers_[i] = std::move(buf);
        mainGeometryDescriptorSets_[i] = gds;
        mainLightingDescriptorSets_[i] = lightingPass_.createDescriptorSet();
    }
};

VulkanGameRenderer::~VulkanGameRenderer() {
    vkDeviceWaitIdle(context_.device);
    for (auto& buf: mainGeometryBuffers_)
        geometryPass_.destroyRenderContext(buf);
}

// ------------------- Public API -------------------

bool VulkanGameRenderer::renderFrame(const GameRenderData& scene) {
    frameManager_.waitForCurrentFrame();

    uint32_t imageIndex = 0;
    if (!frameManager_.acquireImage(imageIndex)) return false;
    VkCommandBuffer cmd = frameManager_.beginFrame();
    recordCommands(cmd, imageIndex, scene);
    frameManager_.endFrame(cmd);
    frameManager_.submit(imageIndex);
    frameManager_.advanceFrame();
    return true;
}

RenderTargetHandle VulkanGameRenderer::createRenderTarget(uint32_t width, uint32_t height) {
    return renderTargetManager_.create(width, height, swapchainColorFormat_);
}

void VulkanGameRenderer::destroyRenderTarget(RenderTargetHandle handle) { renderTargetManager_.destroy(handle); }

// ------------------- Frame Rendering -------------------

void VulkanGameRenderer::recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const GameRenderData& scene) {
    // Set current frame resources
    size_t frameIdx = frameManager_.currentFrameIndex();
    currentFrameLightingDescriptorSet_ = mainLightingDescriptorSets_[frameIdx];
    currentFrameGeometryDescriptorSet_ = mainGeometryDescriptorSets_[frameIdx];
    currentFrameGeometryBuffer_ = &mainGeometryBuffers_[frameIdx];

    VulkanRenderTarget swapchainTarget = swapchainManager_.getSwapchainTarget(imageIndex);
    graph_->setImportedImage(colorTargetHandle_,
                             swapchainTarget.image,
                             swapchainTarget.view,
                             {swapchainTarget.width, swapchainTarget.height});
    graph_->resetLayout(colorTargetHandle_);

    executeRenderGraph(
            cmd, swapchainTarget.image, swapchainTarget.view, {swapchainTarget.width, swapchainTarget.height}, scene);
}

void VulkanGameRenderer::executeRenderGraph(
        VkCommandBuffer cmd, VkImage colorImage, VkImageView colorView, VkExtent2D extent, const GameRenderData& ctx) {
    // Create GameRenderData and execute
    GameRenderData gameData = ctx;
    graph_->execute(cmd, gameData);

    // Transition to present layout
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = colorImage;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};


    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);
}

// ------------------- Render Graph Setup -------------------

void VulkanGameRenderer::setupRenderGraph(VkFormat colorFormat, VkImageUsageFlags colorUsage, VkExtent2D extent) {
    graph_.emplace(context_, extent);

    // --- Import main color target
    colorTargetHandle_ = graph_->importImage("colorTarget", colorFormat, colorUsage);

    // --- Create G-Buffer resources (used by geometry + lighting)
    gbufferAlbedoHandle_ =
            graph_->createTransientImage("gbuffer_albedo",
                                         VK_FORMAT_R8G8B8A8_UNORM,
                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    gbufferNormalHandle_ = graph_->createTransientImage(
            "gbuffer_normal", VK_FORMAT_R16G16_SNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    gbufferDepthHandle_ =
            graph_->createTransientImage("gbuffer_depth",
                                         VK_FORMAT_D32_SFLOAT,
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    // --- Geometry pass ---
    {
        VulkanRenderGraph<GameRenderData>::RenderPassNode n;
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
                           const VulkanRenderGraph<GameRenderData>& graph,
                           const GameRenderData& ctx) -> bool {
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
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
                                 nullptr);
            return true;
        };
        graph_->addPass(std::move(n));
    }

    // --- Lighting pass ---
    {
        VulkanRenderGraph<GameRenderData>::RenderPassNode n;
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
                    VK_IMAGE_ASPECT_COLOR_BIT},
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
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<GameRenderData>& graph,
                           const GameRenderData& ctx) -> bool {
            if (!settings_.enableLighting) return false;
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            lightingPass_.record(cmd,
                                 currentFrameLightingDescriptorSet_,
                                 graph.getImageView(colorTargetHandle_),
                                 extent,
                                 graph.getImageView(gbufferAlbedoHandle_),
                                 graph.getSampler(gbufferAlbedoHandle_),
                                 graph.getImageView(gbufferNormalHandle_),
                                 graph.getSampler(gbufferNormalHandle_),
                                 0, // fbX
                                 0, // fbY
                                 static_cast<int>(extent.width), // fbW
                                 static_cast<int>(extent.height), // fbH
                                 static_cast<float>(extent.width),
                                 static_cast<float>(extent.height),
                                 settings_.enableLighting);
            return true;
        };
        graph_->addPass(std::move(n));
    }

    graph_->build();
}
