#pragma once
#include <log4cxx/logger.h>
#include <typeindex>
#include <typeinfo>
#include "ScriptInstanceContext.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/scene/EntityHandle.hpp"
#include "modules/scene/World.hpp"

class BehaviourComponent {
public:
    virtual ~BehaviourComponent() = default;

    BehaviourComponent(EntityHandle handle, ScriptInstanceContext* ctx) :
        entityHandle_(handle),
        ctx_(ctx),
        logger_(log4cxx::Logger::getLogger(typeid(*this).name())) {}

    virtual void onStart() {}
    virtual void onUpdate(float dt) {}

    World* getWorld() const { return ctx_->world; }
    EntityHandle getEntity() const { return entityHandle_; }

    bool isKeyDown(int key) const { return ctx_->inputSystem && ctx_->inputSystem->isKeyDown(key); }
    bool isKeyPressed(int key) const { return ctx_->inputSystem && ctx_->inputSystem->isKeyPressed(key); }
    bool isMouseButtonDown(int button) const {
        return ctx_->inputSystem && ctx_->inputSystem->isMouseButtonDown(button);
    }
    std::pair<double, double> getMousePosition() const {
        return ctx_->inputSystem ? ctx_->inputSystem->getMousePosition() : std::pair<double, double>{};
    }
    std::pair<double, double> getMouseDelta() const {
        return ctx_->inputSystem ? ctx_->inputSystem->getMouseDelta() : std::pair<double, double>{};
    }
    double getScrollDelta() const { return ctx_->inputSystem ? ctx_->inputSystem->getScrollDelta() : 0.0; }
    void lockMouse() const {
        if (ctx_->inputSystem) ctx_->inputSystem->lockMouse();
    }
    void unlockMouse() const {
        if (ctx_->inputSystem) ctx_->inputSystem->unlockMouse();
    }

    void logDebug(const std::string& msg) const { LOG4CXX_DEBUG(logger_, msg); }
    void logInfo(const std::string& msg) const { LOG4CXX_INFO(logger_, msg); }
    void logWarn(const std::string& msg) const { LOG4CXX_WARN(logger_, msg); }
    void logError(const std::string& msg) const { LOG4CXX_ERROR(logger_, msg); }

    template<typename T>
    T* getComponent() const {
        auto* actor = ctx_->world->getActor(entityHandle_);
        if (actor) {
            if (auto* c = actor->getComponent<T>()) return c;
        }
        if (ctx_->findScriptInstance) {
            return static_cast<T*>(ctx_->findScriptInstance(std::type_index(typeid(T)), entityHandle_));
        }
        return nullptr;
    }

protected:
    EntityHandle entityHandle_;
    ScriptInstanceContext* ctx_ = nullptr;

private:
    log4cxx::LoggerPtr logger_;
};

#define DEFINE_BEHAVIOUR(BehaviourClass)                                                                               \
    extern "C" {                                                                                                       \
    BehaviourComponent* create_##BehaviourClass(EntityHandle handle, ScriptInstanceContext* ctx) {                     \
        return new BehaviourClass(handle, ctx);                                                                        \
    }                                                                                                                  \
    }
