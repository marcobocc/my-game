#pragma once
#include "core/objects/ComponentStorage.hpp"

class GameObject {
public:
    GameObject(std::string name, ComponentStorage& componentStorage) :
        componentStorage_(componentStorage),
        name(std::move(name)) {}

    template<typename T>
    T& add() const {
        return componentStorage_.addComponent<T>(name);
    }

private:
    ComponentStorage& componentStorage_;
    std::string name;
};
