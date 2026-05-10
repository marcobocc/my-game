#pragma once
#include <bitset>
#include <nlohmann/json.hpp>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "ComponentArray.hpp"
#include "EntityMetadata.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"

using EntityHandle = uint64_t;
static constexpr EntityHandle INVALID_HANDLE = std::numeric_limits<EntityHandle>::max();

namespace detail {
    template<typename... Components>
    class EntityManager {
    public:
        static constexpr size_t MAX_COMPONENTS = 64;
        using ComponentMask = std::bitset<MAX_COMPONENTS>;

        static inline size_t nextComponentId = 0;

        template<typename T>
        static size_t getComponentId() {
            static size_t id = nextComponentId++;
            return id;
        }

        // --------------------------------------------------
        // Entity lifecycle
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
            uint32_t location = 0;
            if (!freeLocationsList_.empty()) {
                location = static_cast<uint32_t>(freeLocationsList_.back());
                freeLocationsList_.pop_back();
                ++allocationMetadata_[location].generation;
                allocationMetadata_[location].metadata = {};
            } else {
                location = static_cast<uint32_t>(allocationMetadata_.size());
                allocationMetadata_.emplace_back();
                componentMasks_.emplace_back();
            }
            EntityAllocationInfo& allocMetadata = allocationMetadata_[location];
            allocMetadata.location = location;
            componentMasks_[location].reset();
            EntityHandle lid = logicalId != INVALID_HANDLE ? logicalId : nextEntityHandle_++;
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
            allocationMetadata_[index].metadata = {};
            allocationMetadata_[index].logicalId = INVALID_HANDLE;
            ++allocationMetadata_[index].generation;
            handlesIndex_.erase(entity);
            freeLocationsList_.push_back(index);
        }

        bool exists(EntityHandle entity) const { return getEntityLocation(entity) != nullptr; }

        std::vector<EntityHandle> getEntities() const {
            std::vector<EntityHandle> result;
            result.reserve(handlesIndex_.size());
            for (const auto& lid: handlesIndex_ | std::views::keys)
                result.push_back(lid);
            return result;
        }

        // --------------------------------------------------
        // Component API
        // --------------------------------------------------

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

        // --------------------------------------------------
        // Metadata access
        // --------------------------------------------------

        EntityMetadata* getMetadata(EntityHandle entity) {
            uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return nullptr;
            return &allocationMetadata_[*entityLocation].metadata;
        }

        const EntityMetadata* getMetadata(EntityHandle entity) const {
            const uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return nullptr;
            return &allocationMetadata_[*entityLocation].metadata;
        }

        // --------------------------------------------------
        // Query system
        // --------------------------------------------------

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

        // --------------------------------------------------
        // Serialization / Deserialization
        // --------------------------------------------------

        nlohmann::json serializeToJson(EntityHandle entity) const {
            nlohmann::json j;
            const uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return j;
            j["metadata"] = {{"name", allocationMetadata_[*entityLocation].metadata.entityName}};
            if (auto* t = getComponent<Transform>(entity)) j["transform"] = t->serialize();
            if (auto* c = getComponent<Camera>(entity)) j["camera"] = c->serialize();
            if (auto* r = getComponent<Renderer>(entity)) j["renderer"] = r->serialize();
            if (auto* b = getComponent<BoxCollider>(entity)) j["boxCollider"] = b->serialize();
            return j;
        }

        EntityHandle upsertFromJson(const nlohmann::json& j, EntityHandle entity = INVALID_HANDLE) {
            if (entity != INVALID_HANDLE && exists(entity)) destroyEntity(entity);
            entity = createEntity(entity);
            uint32_t* entityLocation = getEntityLocation(entity);
            if (!entityLocation) return INVALID_HANDLE;
            const auto& meta = j.value("metadata", nlohmann::json{});
            allocationMetadata_[*entityLocation].metadata.entityName = meta.value("name", "");
            if (j.contains("transform")) upsertComponent<Transform>(entity, Transform::deserialize(j["transform"]));
            if (j.contains("camera")) upsertComponent<Camera>(entity, Camera::deserialize(j["camera"]));
            if (j.contains("renderer")) upsertComponent<Renderer>(entity, Renderer::deserialize(j["renderer"]));
            if (j.contains("boxCollider"))
                upsertComponent<BoxCollider>(entity, BoxCollider::deserialize(j["boxCollider"]));
            return entity;
        }

    private:
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
            EntityMetadata metadata;
        };

        std::tuple<ComponentArray<Components>...> componentStorages_;
        std::vector<ComponentMask> componentMasks_;
        std::vector<EntityAllocationInfo> allocationMetadata_;
        std::vector<uint64_t> freeLocationsList_;

        EntityHandle nextEntityHandle_ = 0;
        std::unordered_map<EntityHandle, uint32_t> handlesIndex_;
    };

} // namespace detail

using EntityManager = detail::EntityManager<Transform, Camera, Renderer, BoxCollider>;
