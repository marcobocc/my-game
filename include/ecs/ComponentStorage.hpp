#pragma once
#include <stdexcept>
#include <unordered_map>
#include <vector>

template<typename Component, typename EntityId>
class ComponentStorage {
public:
    void addComponent(EntityId entity, const Component& comp) {
        if (entityToIndex_.contains(entity)) return;
        size_t index = components_.size();
        components_.push_back(comp);
        entityToIndex_[entity] = index;
        indexToEntity_[index] = entity;
    }

    void removeComponent(EntityId entity) {
        auto it = entityToIndex_.find(entity);
        if (it == entityToIndex_.end()) return;

        size_t index = it->second;
        size_t lastIndex = components_.size() - 1;

        // Swap with last element to keep array dense
        components_[index] = components_[lastIndex];
        EntityId lastEntity = indexToEntity_[lastIndex];
        entityToIndex_[lastEntity] = index;
        indexToEntity_[index] = lastEntity;

        components_.pop_back();
        entityToIndex_.erase(entity);
        indexToEntity_.erase(lastIndex);
    }

    Component& getComponent(EntityId entity) {
        auto it = entityToIndex_.find(entity);
        if (it == entityToIndex_.end()) throw std::runtime_error("Component not found");
        return components_[it->second];
    }

    bool hasComponent(EntityId entity) { return entityToIndex_.contains(entity); }
    std::vector<Component>& getAllComponents() { return components_; }
    const std::vector<Component>& getAllComponents() const { return components_; }

private:
    std::vector<Component> components_;
    std::unordered_map<EntityId, size_t> entityToIndex_;
    std::unordered_map<size_t, EntityId> indexToEntity_;
};
