#pragma once
#include "GameRenderData.hpp"

struct IGameRenderer {
    virtual bool renderFrame(const GameRenderData& rd) = 0;
    virtual void setDeltaTime(float dt) = 0;
    virtual ~IGameRenderer() = default;
};
