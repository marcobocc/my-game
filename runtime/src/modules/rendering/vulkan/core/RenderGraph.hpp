#pragma once
#include <cassert>
#include <functional>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "utils/memory.hpp"
#include "utils/structs.hpp"

struct ResourceHandle {
    uint32_t index = UINT32_MAX;
    bool isValid() const { return index != UINT32_MAX; }
    bool operator==(const ResourceHandle&) const = default;
};

struct ResourceUsage {
    ResourceHandle handle = {};
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags accessMask = 0;
    VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
};

struct ResourceDesc {
    std::string name;
    bool imported = false;
    bool isCubemap = false;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags usageFlags = 0;
    uint32_t width = 0; // 0 = swapchain extent
    uint32_t height = 0;

    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE; // full cube view (for sampling)
    std::array<VkImageView, 6> faceViews{}; // per-face views (for rendering into)
    VkSampler sampler = VK_NULL_HANDLE;

    // Per-frame tracking state — reset each frame for imported swapchain images.
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags lastAccess = 0;
    VkPipelineStageFlags lastStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
};

// Generic render graph that works with any render data type.
template<typename RenderData>
class VulkanRenderGraph {
public:
    // Nested render pass node definition
    struct RenderPassNode {
        std::string name;

        std::vector<ResourceUsage> reads;
        std::vector<ResourceUsage> writes;

        // Layout the pass leaves a resource in after execute(), when it differs from
        // the declared write layout (e.g. objectId COLOR_ATTACHMENT → TRANSFER_SRC).
        // The graph updates its tracking state from this without emitting a barrier.
        struct EpilogueUsage {
            ResourceHandle handle = {};
            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkAccessFlags accessMask = 0;
            VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        };
        std::vector<EpilogueUsage> epilogues;

        std::function<bool(VkCommandBuffer, const VulkanRenderGraph&, const RenderData&)> execute;
        std::function<void(VkCommandBuffer, const VulkanRenderGraph&, const RenderData&)> postExecute;
    };

    VulkanRenderGraph(const VulkanContext& context, VkExtent2D initialExtent) :
        context_(context),
        currentExtent_(initialExtent) {}

    ~VulkanRenderGraph() { teardown(); }

    ResourceHandle importImage(const std::string& name, VkFormat format, VkImageUsageFlags usageFlags) {
        ResourceHandle h{static_cast<uint32_t>(resources_.size())};
        ResourceDesc desc{};
        desc.name = name;
        desc.imported = true;
        desc.format = format;
        desc.usageFlags = usageFlags;
        resources_.push_back(desc);
        return h;
    }

    ResourceHandle createTransientImage(const std::string& name,
                                        VkFormat format,
                                        VkImageUsageFlags usageFlags,
                                        uint32_t width = 0,
                                        uint32_t height = 0) {
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

    // Creates a fixed-size cube (6-layer) image. The main imageView is a CUBE view for sampling;
    // getFaceView(handle, face) returns per-face views for rendering into.
    ResourceHandle
    createTransientCubemap(const std::string& name, VkFormat format, VkImageUsageFlags usageFlags, uint32_t size) {
        ResourceHandle h{static_cast<uint32_t>(resources_.size())};
        ResourceDesc desc{};
        desc.name = name;
        desc.imported = false;
        desc.isCubemap = true;
        desc.format = format;
        desc.usageFlags = usageFlags;
        desc.width = size;
        desc.height = size;
        resources_.push_back(desc);
        return h;
    }

    void addPass(RenderPassNode node) { passes_.push_back(std::move(node)); }

    void build() {
        assert(!built_);
        sortPasses();
        allocateTransientImages();
        built_ = true;
    }

    void teardown() {
        if (!built_) return;
        freeTransientImages();
        built_ = false;
    }

    void onResize(uint32_t width, uint32_t height) {
        currentExtent_ = {width, height};
        freeTransientImages();
        // Resources with explicit fixed size (width > 0) keep their dimensions across resizes.
        // Resources with width == 0 implicitly use currentExtent_ at allocation time.
        allocateTransientImages();
    }

    // Must be called before execute() to supply the current target image.
    // extent must match the actual image dimensions so getWidth/getHeight return the correct values.
    void setImportedImage(ResourceHandle handle, VkImage image, VkImageView imageView, VkExtent2D extent) {
        auto& res = resource(handle);
        res.image = image;
        res.imageView = imageView;
        res.width = extent.width;
        res.height = extent.height;
    }

    // Reset per-frame layout tracking for a handle (call for swapchain color each frame).
    void resetLayout(ResourceHandle handle) {
        auto& res = resource(handle);
        res.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        res.lastAccess = 0;
        res.lastStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    void resetAllResourceLayouts() {
        for (auto& res: resources_) {
            res.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            res.lastAccess = 0;
            res.lastStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
    }

    void execute(VkCommandBuffer cmd, const RenderData& renderData) {
        assert(built_);
        for (size_t i: sortedOrder_) {
            RenderPassNode& pass = passes_[i];

            for (const auto& usage: pass.writes)
                emitBarrier(cmd, resource(usage.handle), usage);
            for (const auto& usage: pass.reads)
                emitBarrier(cmd, resource(usage.handle), usage);

            const bool ran = !pass.execute || pass.execute(cmd, *this, renderData);

            if (ran) {
                for (const auto& ep: pass.epilogues) {
                    auto& res = resource(ep.handle);
                    res.currentLayout = ep.layout;
                    res.lastAccess = ep.accessMask;
                    res.lastStage = ep.stageMask;
                }
            }

            if (ran && pass.postExecute) pass.postExecute(cmd, *this, renderData);
        }
    }

    VkImage getImage(ResourceHandle handle) const { return resource(handle).image; }

    VkImageView getImageView(ResourceHandle handle) const { return resource(handle).imageView; }

    // Returns the per-face view for a cubemap resource (face 0..5 = +X,-X,+Y,-Y,+Z,-Z).
    VkImageView getFaceView(ResourceHandle handle, uint32_t face) const {
        assert(resource(handle).isCubemap && face < 6);
        return resource(handle).faceViews[face];
    }

    VkSampler getSampler(ResourceHandle handle) const { return resource(handle).sampler; }

    uint32_t getWidth(ResourceHandle handle) const {
        const auto& res = resource(handle);
        if (res.width > 0) return res.width;
        return currentExtent_.width;
    }

    uint32_t getHeight(ResourceHandle handle) const {
        const auto& res = resource(handle);
        if (res.height > 0) return res.height;
        return currentExtent_.height;
    }

private:
    void allocateTransientImages() {
        const auto& extent = currentExtent_;

        for (auto& res: resources_) {
            if (res.imported) continue;

            const uint32_t w = res.width > 0 ? res.width : extent.width;
            const uint32_t h = res.height > 0 ? res.height : extent.height;

            const bool isDepth = res.format == VK_FORMAT_D32_SFLOAT || res.format == VK_FORMAT_D24_UNORM_S8_UINT ||
                                 res.format == VK_FORMAT_D16_UNORM;
            const VkImageAspectFlags aspect = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            const uint32_t layers = res.isCubemap ? 6u : 1u;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent = {w, h, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = layers;
            imageInfo.format = res.format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = res.usageFlags;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (res.isCubemap) imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
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
            viewInfo.format = res.format;
            viewInfo.subresourceRange.aspectMask = aspect;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = layers;

            if (res.isCubemap) {
                // Full cube view for sampling in shaders.
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
                vkCreateImageView(context_.device, &viewInfo, nullptr, &res.imageView);

                // Per-face views for rendering into each face.
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.subresourceRange.layerCount = 1;
                for (uint32_t face = 0; face < 6; ++face) {
                    viewInfo.subresourceRange.baseArrayLayer = face;
                    vkCreateImageView(context_.device, &viewInfo, nullptr, &res.faceViews[face]);
                }
            } else {
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                vkCreateImageView(context_.device, &viewInfo, nullptr, &res.imageView);
            }

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

    void freeTransientImages() {
        for (auto& res: resources_) {
            if (res.imported) continue;
            if (res.sampler != VK_NULL_HANDLE) {
                vkDestroySampler(context_.device, res.sampler, nullptr);
                res.sampler = VK_NULL_HANDLE;
            }
            if (res.isCubemap) {
                for (auto& fv: res.faceViews) {
                    if (fv != VK_NULL_HANDLE) {
                        vkDestroyImageView(context_.device, fv, nullptr);
                        fv = VK_NULL_HANDLE;
                    }
                }
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

    void sortPasses() {
        const size_t n = passes_.size();

        std::unordered_map<uint32_t, size_t> lastWriter;
        std::vector<std::unordered_set<size_t>> adj(n);
        std::vector<size_t> inDegree(n, 0);

        auto addEdge = [&](size_t producer, size_t consumer) {
            if (producer == consumer) return;
            if (adj[producer].insert(consumer).second) ++inDegree[consumer];
        };

        for (size_t i = 0; i < n; ++i) {
            for (const auto& usage: passes_[i].reads) {
                auto it = lastWriter.find(usage.handle.index);
                if (it != lastWriter.end()) addEdge(it->second, i);
            }
            for (const auto& usage: passes_[i].writes) {
                auto it = lastWriter.find(usage.handle.index);
                if (it != lastWriter.end()) addEdge(it->second, i);
            }

            for (const auto& usage: passes_[i].writes)
                lastWriter[usage.handle.index] = i;
            for (const auto& ep: passes_[i].epilogues)
                lastWriter[ep.handle.index] = i;
        }

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

    static void emitBarrier(VkCommandBuffer cmd, ResourceDesc& res, const ResourceUsage& usage) {
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
        const uint32_t layers = res.isCubemap ? 6u : 1u;
        barrier.subresourceRange = {usage.aspectMask, 0, 1, 0, layers};

        vkCmdPipelineBarrier(cmd, res.lastStage, usage.stageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        res.currentLayout = usage.layout;
        res.lastAccess = usage.accessMask;
        res.lastStage = usage.stageMask;
    }

    ResourceDesc& resource(ResourceHandle h) { return resources_[h.index]; }
    const ResourceDesc& resource(ResourceHandle h) const { return resources_[h.index]; }

    const VulkanContext& context_;
    VkExtent2D currentExtent_;

    std::vector<ResourceDesc> resources_;
    std::vector<RenderPassNode> passes_;
    std::vector<size_t> sortedOrder_;

    bool built_ = false;
};
