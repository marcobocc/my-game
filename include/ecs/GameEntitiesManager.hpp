#pragma once
#include <algorithm>
#include <bitset>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace detail {
    template<typename... Components>
    class GameEntitiesManager {
    public:
        using EntityId = uint64_t;
        static constexpr size_t MaxComponents = 64;
        using ComponentMask = std::bitset<MaxComponents>;

        EntityId createEntity() {
            EntityId id = nextEntityId_++;
            entityMasks_[id] = {};
            entities_.push_back(id);
            return id;
        }

        void destroyEntity(EntityId entity) {
            removeAllComponents(entity);
            entityMasks_.erase(entity);
            auto it = std::ranges::find(entities_, entity);
            if (it != entities_.end()) entities_.erase(it);
        }

        template<typename Component>
        void addComponent(EntityId entity, const Component& component) {
            getComponentMap<Component>()[entity] = component;
            entityMasks_[entity].set(getComponentTypeId<Component>());
        }

        template<typename Component>
        void removeComponent(EntityId entity) {
            getComponentMap<Component>().erase(entity);
            entityMasks_[entity].reset(getComponentTypeId<Component>());
        }

        template<typename Component>
        Component& getComponent(EntityId entity) {
            return getComponentMap<Component>().at(entity);
        }

        template<typename Component>
        bool hasComponent(EntityId entity) const {
            auto it = entityMasks_.find(entity);
            return it != entityMasks_.end() && it->second.test(getComponentTypeId<Component>());
        }

        template<typename... QueryComponents>
        auto query() {
            using Result = std::tuple<EntityId, QueryComponents&...>;
            std::vector<Result> results;
            ComponentMask mask;
            (mask.set(getComponentTypeId<QueryComponents>()), ...);
            for (EntityId entity: entities_) {
                auto it = entityMasks_.find(entity);
                if (it == entityMasks_.end()) continue;
                if ((it->second & mask) == mask) {
                    results.emplace_back(entity, getComponentMap<QueryComponents>().at(entity)...);
                }
            }
            return results;
        }

    private:
        void removeAllComponents(EntityId entity) { (getComponentMap<Components>().erase(entity), ...); }

        template<typename Component>
        std::unordered_map<EntityId, Component>& getComponentMap() {
            return std::get<std::unordered_map<EntityId, Component>>(componentMaps_);
        }

        template<typename Component>
        const std::unordered_map<EntityId, Component>& getComponentMap() const {
            return std::get<std::unordered_map<EntityId, Component>>(componentMaps_);
        }

        template<typename>
        static size_t getComponentTypeId() {
            static size_t id = nextComponentTypeId_++;
            return id;
        }

        static inline size_t nextComponentTypeId_ = 0;
        EntityId nextEntityId_ = 0;
        std::vector<EntityId> entities_;
        std::unordered_map<EntityId, ComponentMask> entityMasks_;
        std::tuple<std::unordered_map<EntityId, Components>...> componentMaps_;
    };
} // namespace detail

#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"
using GameEntitiesManager = detail::GameEntitiesManager<MeshComponent, MaterialComponent>;
