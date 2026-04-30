#pragma once
#include <functional>
#include <string>
#include <vector>

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

class RenderGraph;

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

    // Main recording work. Called after the graph has emitted pre-pass barriers.
    std::function<void(VkCommandBuffer, const RenderGraph&)> execute;

    // Called after execute(), before barriers for the next pass.
    // Used for work that must run while an intermediate layout is live
    // (e.g. picking readback while objectId image is in TRANSFER_SRC).
    std::function<void(VkCommandBuffer, const RenderGraph&)> postExecute;
};
