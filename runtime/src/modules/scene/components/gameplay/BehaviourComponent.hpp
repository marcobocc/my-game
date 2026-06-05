#pragma once
#include <log4cxx/logger.h>
#include <typeinfo>
#include "modules/input/InputSystem.hpp"
#include "modules/scene/EntityHandle.hpp"
#include "modules/scene/World.hpp"
#include "modules/scene/components/IComponent.hpp"

class BehaviourComponent : public IComponent {
public:
    void init(EntityHandle handle, World* world, InputSystem* input) {
        entityHandle_ = handle;
        world_ = world;
        input_ = input;
        logger_ = log4cxx::Logger::getLogger(typeid(*this).name());
    }

    bool allowMultiple() const override { return true; }
    virtual void onStart() = 0;
    virtual void onUpdate(float /*dt*/) = 0;

    World* getWorld() const { return world_; }
    EntityHandle getEntity() const { return entityHandle_; }

    bool isKeyDown(int key) const { return input_ && input_->isKeyDown(key); }
    bool isKeyPressed(int key) const { return input_ && input_->isKeyPressed(key); }
    bool isMouseButtonDown(int btn) const { return input_ && input_->isMouseButtonDown(btn); }
    std::pair<double, double> getMousePosition() const {
        return input_ ? input_->getMousePosition() : std::pair<double, double>{};
    }
    std::pair<double, double> getMouseDelta() const {
        return input_ ? input_->getMouseDelta() : std::pair<double, double>{};
    }
    double getScrollDelta() const { return input_ ? input_->getScrollDelta() : 0.0; }
    void lockMouse() const {
        if (input_) input_->lockMouse();
    }
    void unlockMouse() const {
        if (input_) input_->unlockMouse();
    }

    void logDebug(const std::string& msg) const { LOG4CXX_DEBUG(logger_, msg); }
    void logInfo(const std::string& msg) const { LOG4CXX_INFO(logger_, msg); }
    void logWarn(const std::string& msg) const { LOG4CXX_WARN(logger_, msg); }
    void logError(const std::string& msg) const { LOG4CXX_ERROR(logger_, msg); }

    template<typename T>
    T* getComponent() const {
        auto* actor = world_->getActor(entityHandle_);
        return actor ? actor->getComponent<T>() : nullptr;
    }

private:
    EntityHandle entityHandle_{};
    World* world_ = nullptr;
    InputSystem* input_ = nullptr;
    log4cxx::LoggerPtr logger_;
};
