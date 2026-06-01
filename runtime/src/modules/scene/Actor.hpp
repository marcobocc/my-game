#pragma once
#include <memory>
#include <vector>
#include "EntityHandle.hpp"
#include "components/IComponent.hpp"

class Actor {
public:
    explicit Actor(EntityHandle handle) : handle_(handle) {}

    EntityHandle handle() const { return handle_; }

    template<typename Component, typename... Args>
    Component* addComponent(Args&&... args) {
        auto component = std::make_unique<Component>(std::forward<Args>(args)...);
        Component* ptr = component.get();
        components_.push_back(std::move(component));
        return ptr;
    }

    template<typename Component>
    void removeComponent(Component* target) {
        auto it = std::find_if(components_.begin(), components_.end(), [target](const std::unique_ptr<IComponent>& c) {
            return c.get() == target;
        });
        if (it != components_.end()) components_.erase(it);
    }

    template<typename Component>
    Component* getComponent() {
        for (auto& c: components_)
            if (auto* typed = dynamic_cast<Component*>(c.get())) return typed;
        return nullptr;
    }

    template<typename Component>
    const Component* getComponent() const {
        for (const auto& c: components_)
            if (const auto* typed = dynamic_cast<const Component*>(c.get())) return typed;
        return nullptr;
    }

    template<typename Component>
    std::vector<Component*> getComponents() {
        std::vector<Component*> result;
        for (auto& c: components_)
            if (auto* typed = dynamic_cast<Component*>(c.get())) result.push_back(typed);
        return result;
    }

    template<typename Component>
    std::vector<const Component*> getComponents() const {
        std::vector<const Component*> result;
        for (const auto& c: components_)
            if (const auto* typed = dynamic_cast<const Component*>(c.get())) result.push_back(typed);
        return result;
    }

    const std::vector<std::unique_ptr<IComponent>>& getComponents() const { return components_; }

private:
    EntityHandle handle_;
    std::vector<std::unique_ptr<IComponent>> components_;
};
