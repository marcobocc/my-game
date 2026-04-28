#pragma once
#include <string>
#include <vector>
#include "RenderPassNode.hpp"
#include "rendering/vulkan/core/structs.hpp"

class VulkanSwapchainManager;

struct ResourceDesc {
    std::string name;
    bool imported = false;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags usageFlags = 0;
    uint32_t width = 0; // 0 = swapchain extent
    uint32_t height = 0;

    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    // Per-frame tracking state — reset each frame for imported swapchain images.
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags lastAccess = 0;
    VkPipelineStageFlags lastStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
};

class RenderGraph {
public:
    RenderGraph(const VulkanContext& context, VulkanSwapchainManager& swapchainManager);
    ~RenderGraph();

    // --- Resource registration  ---

    // Import an image the graph does not own (swapchain color/depth).
    // The actual VkImage is set per-frame via setImportedImage().
    ResourceHandle importImage(const std::string& name, VkFormat format, VkImageUsageFlags usageFlags);

    // Declare a transient image the graph allocates and owns.
    ResourceHandle createTransientImage(const std::string& name,
                                        VkFormat format,
                                        VkImageUsageFlags usageFlags,
                                        uint32_t width = 0,
                                        uint32_t height = 0);

    // --- Pass registration ---
    void addPass(RenderPassNode node);

    // --- Lifecycle ---
    void build();
    void teardown();
    void onResize(uint32_t width, uint32_t height);

    // --- Per-frame ---

    // Must be called before execute() to supply the current swapchain image.
    void setImportedImage(ResourceHandle handle, VkImage image, VkImageView imageView);

    // Reset per-frame layout tracking for a handle (call for swapchain color each frame).
    void resetLayout(ResourceHandle handle);

    void execute(VkCommandBuffer cmd);

    // --- Resource accessors (for pass lambdas) ---
    VkImage getImage(ResourceHandle handle) const;
    VkImageView getImageView(ResourceHandle handle) const;
    VkSampler getSampler(ResourceHandle handle) const;
    uint32_t getWidth(ResourceHandle handle) const;
    uint32_t getHeight(ResourceHandle handle) const;

private:
    void allocateTransientImages();
    void freeTransientImages();
    void sortPasses();

    void emitBarrier(VkCommandBuffer cmd, ResourceDesc& res, const ResourceUsage& usage) const;

    ResourceDesc& resource(ResourceHandle h) { return resources_[h.index]; }
    const ResourceDesc& resource(ResourceHandle h) const { return resources_[h.index]; }

    const VulkanContext& context_;
    VulkanSwapchainManager& swapchainManager_;

    std::vector<ResourceDesc> resources_;
    std::vector<RenderPassNode> passes_;
    std::vector<size_t> sortedOrder_;

    bool built_ = false;
};
