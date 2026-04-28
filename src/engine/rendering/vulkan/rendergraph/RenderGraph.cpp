#include "RenderGraph.hpp"
#include <cassert>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "rendering/vulkan/core/memory.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"

RenderGraph::RenderGraph(const VulkanContext& context, VulkanSwapchainManager& swapchainManager) :
    context_(context),
    swapchainManager_(swapchainManager) {}

RenderGraph::~RenderGraph() { teardown(); }

// ------------------- Resource registration -------------------

ResourceHandle RenderGraph::importImage(const std::string& name, VkFormat format, VkImageUsageFlags usageFlags) {
    ResourceHandle h{static_cast<uint32_t>(resources_.size())};
    ResourceDesc desc{};
    desc.name = name;
    desc.imported = true;
    desc.format = format;
    desc.usageFlags = usageFlags;
    resources_.push_back(desc);
    return h;
}

ResourceHandle RenderGraph::createTransientImage(
        const std::string& name, VkFormat format, VkImageUsageFlags usageFlags, uint32_t width, uint32_t height) {
    ResourceHandle h{static_cast<uint32_t>(resources_.size())};
    ResourceDesc desc{};
    desc.name = name;
    desc.imported = false;
    desc.format = format;
    desc.usageFlags = usageFlags;
    desc.width = width;
    desc.height = height;
    resources_.push_back(desc);
    return h;
}

// ------------------- Pass registration -------------------

void RenderGraph::addPass(RenderPassNode node) { passes_.push_back(std::move(node)); }

// ------------------- Lifecycle -------------------

void RenderGraph::build() {
    assert(!built_);
    sortPasses();
    allocateTransientImages();
    built_ = true;
}

void RenderGraph::teardown() {
    if (!built_) return;
    freeTransientImages();
    built_ = false;
}

void RenderGraph::onResize(uint32_t width, uint32_t height) {
    freeTransientImages();
    for (auto& res: resources_) {
        if (!res.imported && res.width > 0) {
            res.width = width;
            res.height = height;
        }
    }
    allocateTransientImages();
}

// ------------------- Per-frame -------------------

void RenderGraph::setImportedImage(ResourceHandle handle, VkImage image, VkImageView imageView) {
    auto& res = resource(handle);
    res.image = image;
    res.imageView = imageView;
}

void RenderGraph::resetLayout(ResourceHandle handle) {
    auto& res = resource(handle);
    res.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    res.lastAccess = 0;
    res.lastStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
}

void RenderGraph::execute(VkCommandBuffer cmd) {
    assert(built_);
    for (size_t i: sortedOrder_) {
        RenderPassNode& pass = passes_[i];

        for (const auto& usage: pass.writes)
            emitBarrier(cmd, resource(usage.handle), usage);
        for (const auto& usage: pass.reads)
            emitBarrier(cmd, resource(usage.handle), usage);

        if (pass.execute) pass.execute(cmd, *this);

        // Update tracking state from epilogues without emitting barriers.
        for (const auto& ep: pass.epilogues) {
            auto& res = resource(ep.handle);
            res.currentLayout = ep.layout;
            res.lastAccess = ep.accessMask;
            res.lastStage = ep.stageMask;
        }

        if (pass.postExecute) pass.postExecute(cmd, *this);
    }
}

// ------------------- Resource accessors -------------------

VkImage RenderGraph::getImage(ResourceHandle handle) const { return resource(handle).image; }
VkImageView RenderGraph::getImageView(ResourceHandle handle) const { return resource(handle).imageView; }
VkSampler RenderGraph::getSampler(ResourceHandle handle) const { return resource(handle).sampler; }

uint32_t RenderGraph::getWidth(ResourceHandle handle) const {
    const auto& res = resource(handle);
    if (res.width > 0) return res.width;
    return swapchainManager_.swapchain().swapchainExtent.width;
}

uint32_t RenderGraph::getHeight(ResourceHandle handle) const {
    const auto& res = resource(handle);
    if (res.height > 0) return res.height;
    return swapchainManager_.swapchain().swapchainExtent.height;
}

// ------------------- Private -------------------

void RenderGraph::sortPasses() {
    const size_t n = passes_.size();

    // Build adjacency list and in-degree table incrementally so that each pass
    // depends on the immediately preceding writer of every resource it reads or
    // writes (write-after-write / read-after-write).  A flat pre-scan would
    // point all readers at the *final* writer, breaking chains like
    // Lighting → Grid → Gizmo → UI that all write the same swapchain image.
    std::unordered_map<uint32_t, size_t> lastWriter; // resource index -> pass index
    std::vector<std::unordered_set<size_t>> adj(n);
    std::vector<size_t> inDegree(n, 0);

    auto addEdge = [&](size_t producer, size_t consumer) {
        if (producer == consumer) return;
        if (adj[producer].insert(consumer).second) ++inDegree[consumer];
    };

    for (size_t i = 0; i < n; ++i) {
        // Read dependency: this pass must run after whoever last wrote the resource.
        for (const auto& usage: passes_[i].reads) {
            auto it = lastWriter.find(usage.handle.index);
            if (it != lastWriter.end()) addEdge(it->second, i);
        }
        // Write-after-write dependency: if another pass already wrote this resource,
        // this pass must run after it so the ordering is well-defined.
        for (const auto& usage: passes_[i].writes) {
            auto it = lastWriter.find(usage.handle.index);
            if (it != lastWriter.end()) addEdge(it->second, i);
        }

        // Update lastWriter *after* edges are built for this pass.
        for (const auto& usage: passes_[i].writes)
            lastWriter[usage.handle.index] = i;
        for (const auto& ep: passes_[i].epilogues)
            lastWriter[ep.handle.index] = i;
    }

    // Kahn's algorithm.
    std::queue<size_t> ready;
    for (size_t i = 0; i < n; ++i)
        if (inDegree[i] == 0) ready.push(i);

    sortedOrder_.clear();
    sortedOrder_.reserve(n);
    while (!ready.empty()) {
        size_t cur = ready.front();
        ready.pop();
        sortedOrder_.push_back(cur);
        for (size_t dep: adj[cur]) {
            if (--inDegree[dep] == 0) ready.push(dep);
        }
    }

    if (sortedOrder_.size() != n) throw std::runtime_error("RenderGraph: cycle detected among render passes");
}

void RenderGraph::allocateTransientImages() {
    const auto& extent = swapchainManager_.swapchain().swapchainExtent;

    for (auto& res: resources_) {
        if (res.imported) continue;

        const uint32_t w = res.width > 0 ? res.width : extent.width;
        const uint32_t h = res.height > 0 ? res.height : extent.height;

        const bool isDepth = (res.format == VK_FORMAT_D32_SFLOAT || res.format == VK_FORMAT_D24_UNORM_S8_UINT ||
                              res.format == VK_FORMAT_D16_UNORM);
        const VkImageAspectFlags aspect = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {w, h, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = res.format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = res.usageFlags;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(context_.device, &imageInfo, nullptr, &res.image);

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(context_.device, res.image, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
                getMemoryType(context_.physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(context_.device, &allocInfo, nullptr, &res.memory);
        vkBindImageMemory(context_.device, res.image, res.memory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = res.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = res.format;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(context_.device, &viewInfo, nullptr, &res.imageView);

        if (res.usageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            vkCreateSampler(context_.device, &samplerInfo, nullptr, &res.sampler);
        }

        res.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        res.lastAccess = 0;
        res.lastStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
}

void RenderGraph::freeTransientImages() {
    for (auto& res: resources_) {
        if (res.imported) continue;
        if (res.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(context_.device, res.sampler, nullptr);
            res.sampler = VK_NULL_HANDLE;
        }
        if (res.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(context_.device, res.imageView, nullptr);
            res.imageView = VK_NULL_HANDLE;
        }
        if (res.image != VK_NULL_HANDLE) {
            vkDestroyImage(context_.device, res.image, nullptr);
            res.image = VK_NULL_HANDLE;
        }
        if (res.memory != VK_NULL_HANDLE) {
            vkFreeMemory(context_.device, res.memory, nullptr);
            res.memory = VK_NULL_HANDLE;
        }
    }
}

void RenderGraph::emitBarrier(VkCommandBuffer cmd, ResourceDesc& res, const ResourceUsage& usage) const {
    if (res.currentLayout == usage.layout && res.lastAccess == usage.accessMask) return;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = res.currentLayout;
    barrier.newLayout = usage.layout;
    barrier.srcAccessMask = res.lastAccess;
    barrier.dstAccessMask = usage.accessMask;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = res.image;
    barrier.subresourceRange = {usage.aspectMask, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmd, res.lastStage, usage.stageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    res.currentLayout = usage.layout;
    res.lastAccess = usage.accessMask;
    res.lastStage = usage.stageMask;
}
