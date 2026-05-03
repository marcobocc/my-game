#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include "systems/rendering/vulkan/core/VulkanPickingBackend.hpp"

class PickingSystem {
public:
    explicit PickingSystem(VulkanPickingBackend& backend) : backend_(backend) {}

    void requestPick(uint32_t x, uint32_t y) { backend_.requestPick(x, y); }

    std::optional<std::string> getPickResult() { return backend_.takePendingResult(); }

private:
    VulkanPickingBackend& backend_;
};
