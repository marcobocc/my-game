#pragma once
#include "ScriptInstanceContext.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/scene/EntityHandle.hpp"
#include "modules/scene/World.hpp"

class BehaviourComponent {
public:
    virtual ~BehaviourComponent() = default;

    BehaviourComponent(EntityHandle handle, ScriptInstanceContext* ctx) : entityHandle_(handle), ctx_(ctx) {}

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

    template<typename T>
    T* getComponent() const {
        auto* actor = ctx_->world->getActor(entityHandle_);
        return actor ? actor->getComponent<T>() : nullptr;
    }

protected:
    EntityHandle entityHandle_;
    ScriptInstanceContext* ctx_ = nullptr;
};

#define DEFINE_BEHAVIOUR(BehaviourClass)                                                                               \
    extern "C" {                                                                                                       \
    BehaviourComponent* create_behaviour(EntityHandle handle, ScriptInstanceContext* ctx) {                            \
        return new BehaviourClass(handle, ctx);                                                                        \
    }                                                                                                                  \
    }
