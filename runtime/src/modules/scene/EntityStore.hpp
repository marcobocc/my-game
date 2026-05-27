#pragma once
#include <bitset>
#include <nlohmann/json.hpp>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "ComponentArray.hpp"
#include "components/BoxCollider.hpp"
#include "components/Camera.hpp"
#include "components/Light.hpp"
#include "components/Metadata.hpp"
#include "components/Renderer.hpp"
#include "components/Transform.hpp"

using EntityHandle = uint64_t;

namespace detail {
    static constexpr EntityHandle INVALID_HANDLE = std::numeric_limits<EntityHandle>::max();

    template<typename... Components>
    class EntityStore {
    public:
        // --------------------------------------------------
        // Mutations
        // --------------------------------------------------

        void clear() {
            allocationMetadata_.clear();
            componentMasks_.clear();
            freeLocationsList_.clear();
            handlesIndex_.clear();
            nextEntityHandle_ = 0;
            (std::get<ComponentArray<Components>>(componentStorages_).clear(), ...);
        }

        EntityHandle createEntity(EntityHandle logicalId = INVALID_HANDLE) {
            EntityHandle lid = logicalId != INVALID_HANDLE ? logicalId : nextEntityHandle_++;
            if (exists(lid)) throw std::runtime_error("Entity already exists");
            uint32_t location = 0;
            if (!freeLocationsList_.empty()) {
                location = static_cast<uint32_t>(freeLocationsList_.back());
                freeLocationsList_.pop_back();
                ++allocationMetadata_[location].generation;
            } else {
                location = static_cast<uint32_t>(allocationMetadata_.size());
                allocationMetadata_.emplace_back();
                componentMasks_.emplace_back();
            }
            EntityAllocationInfo& allocMetadata = allocationMetadata_[location];
            allocMetadata.location = location;
            componentMasks_[location].reset();
            allocMetadata.logicalId = lid;
            handlesIndex_[lid] = location;
            return lid;
        }

        void destroyEntity(EntityHandle entity) {
            uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return;
            uint32_t index = *entityLocation;
            (std::get<ComponentArray<Components>>(componentStorages_).remove(index), ...);
            componentMasks_[index].reset();
            allocationMetadata_[index].logicalId = INVALID_HANDLE;
            ++allocationMetadata_[index].generation;
            handlesIndex_.erase(entity);
            freeLocationsList_.push_back(index);
        }

        template<typename Component>
        Component* upsertComponent(EntityHandle entity, const Component& value = Component{}) {
            uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return nullptr;
            auto& componentStorage = std::get<ComponentArray<Component>>(componentStorages_);
            Component* c = componentStorage.get(*entityLocation);
            if (c) {
                *c = value;
                return c;
            }
            c = componentStorage.add(*entityLocation, value);
            componentMasks_[*entityLocation].set(getComponentId<Component>());
            return c;
        }

        template<typename Component>
        void removeComponent(EntityHandle entity) {
            uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return;
            if (!componentMasks_[*entityLocation].test(getComponentId<Component>())) return;
            std::get<ComponentArray<Component>>(componentStorages_).remove(*entityLocation);
            componentMasks_[*entityLocation].reset(getComponentId<Component>());
        }

        // --------------------------------------------------
        // Queries
        // --------------------------------------------------

        bool exists(EntityHandle entity) const { return getEntityLocation(entity) != nullptr; }

        std::vector<EntityHandle> getEntities() const {
            std::vector<EntityHandle> result;
            result.reserve(handlesIndex_.size());
            for (const auto& lid: handlesIndex_ | std::views::keys)
                result.push_back(lid);
            return result;
        }

        template<typename Component>
        Component* getComponent(EntityHandle entity) {
            uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return nullptr;
            return std::get<ComponentArray<Component>>(componentStorages_).get(*entityLocation);
        }

        template<typename Component>
        const Component* getComponent(EntityHandle entity) const {
            const uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return nullptr;
            return std::get<ComponentArray<Component>>(componentStorages_).get(*entityLocation);
        }

        template<typename Component>
        bool hasComponent(EntityHandle entity) const {
            const uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return false;
            return componentMasks_[*entityLocation].test(getComponentId<Component>());
        }

        template<typename Func>
        void forEachComponent(EntityHandle entity, Func&& fn) {
            (
                    [&] {
                        if (auto* component = getComponent<Components>(entity)) fn(*component);
                    }(),
                    ...);
        }

        template<typename... QueryComponents>
        auto query() const {
            std::vector<std::tuple<EntityHandle, const QueryComponents*...>> result;
            ComponentMask mask;
            (mask.set(getComponentId<QueryComponents>()), ...);
            for (const auto& [lid, index]: handlesIndex_) {
                if ((componentMasks_[index] & mask) == mask)
                    result.emplace_back(lid,
                                        std::get<ComponentArray<QueryComponents>>(componentStorages_).get(index)...);
            }
            return result;
        }

    private:
        static constexpr size_t MAX_COMPONENTS = 64;
        using ComponentMask = std::bitset<MAX_COMPONENTS>;

        static inline size_t nextComponentId = 0;

        template<typename>
        static size_t getComponentId() {
            static size_t id = nextComponentId++;
            return id;
        }

        template<typename Component>
        ComponentArray<Component>& getComponentStorage() {
            return std::get<ComponentArray<Component>>(componentStorages_);
        }

        uint32_t* getEntityLocation(EntityHandle entity) {
            auto it = handlesIndex_.find(entity);
            if (it == handlesIndex_.end()) return nullptr;
            return &it->second;
        }

        const uint32_t* getEntityLocation(EntityHandle entity) const {
            auto it = handlesIndex_.find(entity);
            if (it == handlesIndex_.end()) return nullptr;
            return &it->second;
        }

        struct EntityAllocationInfo {
            uint32_t location = UINT32_MAX;
            uint32_t generation = 0;
            EntityHandle logicalId = INVALID_HANDLE;
        };

        std::tuple<ComponentArray<Components>...> componentStorages_;
        std::vector<ComponentMask> componentMasks_;
        std::vector<EntityAllocationInfo> allocationMetadata_;
        std::vector<uint64_t> freeLocationsList_;

        EntityHandle nextEntityHandle_ = 0;
        std::unordered_map<EntityHandle, uint32_t> handlesIndex_;
    };

} // namespace detail

using EntityManager = detail::EntityStore<Metadata, Transform, Camera, Renderer, BoxCollider, Light>;
