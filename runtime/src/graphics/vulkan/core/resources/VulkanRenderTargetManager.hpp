#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "../../../RenderTargetHandle.hpp"
#include "../utils/images.hpp"
#include "../utils/structs.hpp"

class VulkanRenderTargetManager {
public:
    explicit VulkanRenderTargetManager(VulkanContext& context) : context_(context) {}

    ~VulkanRenderTargetManager() { renderTargets_.clear(); }

    RenderTargetHandle create(uint32_t width, uint32_t height, VkFormat swapchainColorFormat) {
        RenderTargetHandle handle{static_cast<uint32_t>(renderTargets_.size())};
        auto rt = std::make_unique<VulkanRenderTarget>();
        rt->width = width;
        rt->height = height;
        rt->format = swapchainColorFormat;

        allocateRenderTargetImages(
                context_.device, context_.physicalDevice, swapchainColorFormat, *rt, rt->colorMemory, rt->colorSampler);

        renderTargets_.push_back(std::move(rt));
        return handle;
    }

    void destroy(RenderTargetHandle handle) {
        if (!handle.isValid() || handle.index >= renderTargets_.size()) return;
        auto& rt = renderTargets_[handle.index];
        if (!rt) return;

        vkDeviceWaitIdle(context_.device);
        freeRenderTargetImages(context_.device, *rt, rt->colorMemory, rt->colorSampler);
        rt.reset();
    }

    VulkanRenderTarget* get(RenderTargetHandle handle) {
        if (!handle.isValid() || handle.index >= renderTargets_.size()) return nullptr;
        return renderTargets_[handle.index].get();
    }

    const VulkanRenderTarget* get(RenderTargetHandle handle) const {
        if (!handle.isValid() || handle.index >= renderTargets_.size()) return nullptr;
        return renderTargets_[handle.index].get();
    }

    const std::vector<std::unique_ptr<VulkanRenderTarget>>& getAllRenderTargets() const { return renderTargets_; }

    void destroyAll() {
        vkDeviceWaitIdle(context_.device);
        for (size_t i = 0; i < renderTargets_.size(); ++i) {
            auto& rt = renderTargets_[i];
            if (rt) {
                freeRenderTargetImages(context_.device, *rt, rt->colorMemory, rt->colorSampler);
            }
        }
        renderTargets_.clear();
    }

private:
    VulkanContext& context_;
    std::vector<std::unique_ptr<VulkanRenderTarget>> renderTargets_;
};
