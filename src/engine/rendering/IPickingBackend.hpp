#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class IPickingBackend {
public:
    virtual ~IPickingBackend() = default;

    virtual void requestPick(uint32_t x, uint32_t y) = 0;

    // Called by the orchestrator after GPU fence signals. Reads back the pixel and stores the result internally.
    virtual void processReadback(const std::vector<std::string>& objectIdMap) = 0;

    // Called by PickingSystem to consume the resolved result (nullopt if nothing pending).
    virtual std::optional<std::string> takePendingResult() = 0;
};
