#pragma once
#include <cstdint>

struct RenderTargetHandle {
    uint32_t index = UINT32_MAX;
    bool isValid() const { return index != UINT32_MAX; }
    bool operator==(const RenderTargetHandle&) const = default;
};
