#include "VulkanGameRenderer.hpp"
#include <volk.h>
#include "core/VulkanFrameManager.hpp"
#include "core/VulkanSwapchainManager.hpp"
#include "core/resources/VulkanRenderTargetManager.hpp"
#include "core/resources/VulkanResourcesManager.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "passes/VulkanGeometryPass.hpp"
#include "passes/VulkanLightingPass.hpp"
#include "passes/VulkanShadowPass.hpp"
#include "passes/VulkanSkyPass.hpp"

VulkanGameRenderer::VulkanGameRenderer(VulkanContext& context,
                                       VulkanFrameManager& frameManager,
                                       VulkanRenderTargetManager& renderTargetManager,
                                       VulkanGeometryPass& geometryPass,
                                       VulkanLightingPass& lightingPass,
                                       VulkanSwapchainManager& swapchainManager,
                                       RendererSettings& settings,
                                       VulkanResourcesManager& resourcesManager,
                                       AssetLoader& assetLoader) :
    context_(context),
    settings_(settings),
    swapchainManager_(swapchainManager),
    frameManager_(frameManager),
    renderTargetManager_(renderTargetManager),
    geometryPass_(geometryPass),
    lightingPass_(lightingPass),
    shadowPass_(std::make_unique<VulkanShadowPass>(resourcesManager, assetLoader)),
    skyPass_(std::make_unique<VulkanSkyPass>(context, assetLoader, resourcesManager)),
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
        mainLightingDescriptorSets_[i] = lightingPass_.createDescriptorSet(0);
    }
};

VulkanGameRenderer::~VulkanGameRenderer() {
    vkDeviceWaitIdle(context_.device);
    for (auto& buf: mainGeometryBuffers_)
        geometryPass_.destroyRenderContext(buf);
}

// ------------------- Public API -------------------

bool VulkanGameRenderer::renderFrame(const GameRenderData& renderData) {
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

RenderTargetHandle VulkanGameRenderer::createRenderTarget(uint32_t width, uint32_t height) {
    return renderTargetManager_.create(width, height, swapchainColorFormat_);
}

void VulkanGameRenderer::destroyRenderTarget(RenderTargetHandle handle) { renderTargetManager_.destroy(handle); }

// ------------------- Frame Rendering -------------------

void VulkanGameRenderer::recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const GameRenderData& renderData) {
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

    executeRenderGraph(cmd,
                       swapchainTarget.image,
                       swapchainTarget.view,
                       {swapchainTarget.width, swapchainTarget.height},
                       renderData);
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

    // --- Shadow map (fixed resolution, depth-only)
    shadowDepthHandle_ =
            graph_->createTransientImage("shadow_depth",
                                         VK_FORMAT_D32_SFLOAT,
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                         VulkanShadowPass::SHADOW_MAP_SIZE,
                                         VulkanShadowPass::SHADOW_MAP_SIZE);

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

    // --- Shadow pass (runs before geometry)
    {
        VulkanRenderGraph<GameRenderData>::RenderPassNode n;
        n.name = "ShadowPass";
        n.writes = {{shadowDepthHandle_,
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                     VK_IMAGE_ASPECT_DEPTH_BIT}};
        n.execute = [this](VkCommandBuffer cmd,
                           const VulkanRenderGraph<GameRenderData>& graph,
                           const GameRenderData& ctx) -> bool {
            shadowPass_->record(cmd,
                                graph.getImageView(shadowDepthHandle_),
                                ctx.drawQueue,
                                ctx.lightsWithTransforms,
                                ctx.camera,
                                ctx.cameraTransform,
                                0);
            return true;
        };
        graph_->addPass(std::move(n));
    }

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
                    VK_IMAGE_ASPECT_DEPTH_BIT},
                   {shadowDepthHandle_,
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
            lightingPass_.updateLights(ctx.lightsWithTransforms);
            lightingPass_.uploadPassHeader(0, /*isFirstLight=*/true);
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            lightingPass_.record(cmd,
                                 currentFrameLightingDescriptorSet_,
                                 graph.getImageView(colorTargetHandle_),
                                 extent,
                                 graph.getImageView(gbufferAlbedoHandle_),
                                 graph.getSampler(gbufferAlbedoHandle_),
                                 graph.getImageView(gbufferNormalHandle_),
                                 graph.getSampler(gbufferNormalHandle_),
                                 graph.getImageView(shadowDepthHandle_),
                                 graph.getSampler(shadowDepthHandle_),
                                 graph.getImageView(gbufferDepthHandle_),
                                 graph.getSampler(gbufferDepthHandle_),
                                 shadowPass_->lightSpaceMatrix(0),
                                 ctx.camera,
                                 ctx.cameraTransform,
                                 0,
                                 0,
                                 static_cast<int>(extent.width),
                                 static_cast<int>(extent.height),
                                 static_cast<float>(extent.width),
                                 static_cast<float>(extent.height),
                                 settings_.enableLighting,
                                 /*isFirstLight=*/true);
            return true;
        };
        graph_->addPass(std::move(n));
    }

    // --- Sky pass (after lighting so LOAD_OP_CLEAR doesn't wipe it; depth test at 1.0 fills sky pixels) ---
    {
        VulkanRenderGraph<GameRenderData>::RenderPassNode n;
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
                           const VulkanRenderGraph<GameRenderData>& graph,
                           const GameRenderData& ctx) -> bool {
            const VkExtent2D extent{graph.getWidth(colorTargetHandle_), graph.getHeight(colorTargetHandle_)};
            skyPass_->record(cmd,
                             graph.getImageView(colorTargetHandle_),
                             graph.getImageView(gbufferDepthHandle_),
                             extent,
                             ctx.camera,
                             ctx.cameraTransform);
            return true;
        };
        graph_->addPass(std::move(n));
    }

    graph_->build();
}
