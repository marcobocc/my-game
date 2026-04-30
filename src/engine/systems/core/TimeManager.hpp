#pragma once
#include <functional>

class TimeManager {
public:
    using ClockProvider = std::function<float()>;

    explicit TimeManager(ClockProvider clockProvider) : clockProvider_(std::move(clockProvider)), lastTime_(0.0f) {}

    void beginFrame() {
        float currentTime = clockProvider_();
        realDeltaTime_ = currentTime - lastTime_;
        lastTime_ = currentTime;
        totalRealTime_ += realDeltaTime_;
        gameDeltaTime_ = realDeltaTime_ * timeScale_;
        totalGameTime_ += gameDeltaTime_;
        accumulator_ += realDeltaTime_;
    }

    void endFrame() {
        if (accumulator_ < 0.0f) accumulator_ = 0.0f;
    }

    bool consumeFixedStep() {
        if (accumulator_ < fixedDeltaTime_) return false;
        accumulator_ -= fixedDeltaTime_;
        return true;
    }

    void setTimeScale(float scale) { timeScale_ = scale; }

    float getTimeScale() const { return timeScale_; }
    float getGameDeltaTime() const { return gameDeltaTime_; }
    static float getFixedDeltaTime() { return fixedDeltaTime_; }
    float interpolationFactor() const { return accumulator_ / fixedDeltaTime_; }

private:
    ClockProvider clockProvider_;
    float lastTime_ = 0.0f;
    float realDeltaTime_ = 0.0f;
    float totalRealTime_ = 0.0f;
    float timeScale_ = 1.0f;
    float gameDeltaTime_ = 0.0f;
    float totalGameTime_ = 0.0f;
    float accumulator_ = 0.0f;
    static constexpr float fixedDeltaTime_ = 1.0f / 60.0f;
};
