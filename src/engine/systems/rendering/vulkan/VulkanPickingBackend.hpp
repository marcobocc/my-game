#pragma once
#include <optional>
#include <string>
#include <vector>
#include <volk.h>
#include "systems/rendering/IPickingBackend.hpp"
#include "systems/rendering/vulkan/utils/buffers.hpp"
#include "systems/rendering/vulkan/utils/memory.hpp"
#include "systems/rendering/vulkan/utils/structs.hpp"

class VulkanPickingBackend : public IPickingBackend {
public:
    explicit VulkanPickingBackend(const VulkanContext& context) : context_(context) { createReadbackBuffer(); }

    ~VulkanPickingBackend() override { destroyBuffer(context_.device, readbackBuffer_); }

    void requestPick(uint32_t x, uint32_t y) override {
        hasPendingPickRequest_ = true;
        pendingPickX_ = x;
        pendingPickY_ = y;
    }

    // Records a copy from the object ID image to the readback buffer if a pick was requested.
    // Called by the orchestrator after VulkanObjectIdPass::record(), while the image is in TRANSFER_SRC layout.
    void recordReadback(VkCommandBuffer cmd, VkImage objectIdBufferImage, uint32_t width, uint32_t height) {
        if (!hasPendingPickRequest_) return;

        uint32_t x = std::min(pendingPickX_, width - 1);
        uint32_t y = std::min(pendingPickY_, height - 1);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
        region.imageExtent = {1, 1, 1};
        vkCmdCopyImageToBuffer(
                cmd, objectIdBufferImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readbackBuffer_.buffer, 1, &region);

        hasPendingPickRequest_ = false;
        pickResultPending_ = true;
    }

    void processReadback(const std::vector<std::string>& objectIdMap) override {
        if (!pickResultPending_) return;
        pickResultPending_ = false;

        uint32_t rawId = 0;
        void* mapped = nullptr;
        vkMapMemory(context_.device, readbackBuffer_.memory, 0, sizeof(uint32_t), 0, &mapped);
        std::memcpy(&rawId, mapped, sizeof(uint32_t));
        vkUnmapMemory(context_.device, readbackBuffer_.memory);

        if (rawId == 0 || rawId == 0xFFFFFFFF || (rawId - 1) >= objectIdMap.size())
            resolvedResult_ = std::nullopt;
        else
            resolvedResult_ = objectIdMap[rawId - 1];
    }

    std::optional<std::string> takePendingResult() override { return std::exchange(resolvedResult_, std::nullopt); }

private:
    void createReadbackBuffer() {
        readbackBuffer_ = createBuffer(context_.device,
                                       context_.physicalDevice,
                                       sizeof(uint32_t),
                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    const VulkanContext& context_;
    VulkanBuffer readbackBuffer_{};

    bool hasPendingPickRequest_ = false;
    uint32_t pendingPickX_ = 0;
    uint32_t pendingPickY_ = 0;
    bool pickResultPending_ = false;
    std::optional<std::string> resolvedResult_;
};
