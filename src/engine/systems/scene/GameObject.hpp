#pragma once
#include "systems/scene/ComponentStorage.hpp"

class GameObject {
public:
    GameObject(std::string name, ComponentStorage& componentStorage) :
        componentStorage_(componentStorage),
        name(std::move(name)) {}

    template<typename T>
    T& add() {
        return componentStorage_.addComponent<T>(name);
    }

    template<typename T>
    T& get() const {
        return componentStorage_.getComponent<T>(name);
    }

    template<typename T>
    bool has() const {
        return componentStorage_.hasComponent<T>(name);
    }

    const std::string& getName() const { return name; }

private:
    ComponentStorage& componentStorage_;
    std::string name;
};
