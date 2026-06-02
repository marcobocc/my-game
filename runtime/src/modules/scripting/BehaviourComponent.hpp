#pragma once
#include "ScriptInstanceContext.hpp"
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
