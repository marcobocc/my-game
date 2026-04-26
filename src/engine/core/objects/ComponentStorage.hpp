#pragma once
#include <algorithm>
#include <bitset>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "core/objects/components/BoxCollider.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Renderer.hpp"
#include "core/objects/components/Transform.hpp"

namespace detail {
    template<typename... Components>
    class ComponentStorage {
    public:
        static constexpr size_t MAX_COMPONENTS = 64;
        using ComponentMask = std::bitset<MAX_COMPONENTS>;

        void destroyAllComponents(const std::string& objectName) { (destroyComponent<Components>(objectName), ...); }

        template<typename Component>
        Component& addComponent(const std::string& objectName) {
            if (!objectMasks_.contains(objectName)) {
                objectMasks_[objectName] = {};
            }
            auto& container = getComponentContainer<Component>();
            auto [it, successful] = container.try_emplace(objectName);
            objectMasks_[objectName].set(getComponentId<Component>());
            return it->second;
        }

        template<typename Component>
        void destroyComponent(const std::string& objectName) {
            getComponentContainer<Component>().erase(objectName);
            objectMasks_[objectName].reset(getComponentId<Component>());
        }

        template<typename Component>
        Component& getComponent(const std::string& objectName) {
            return getComponentContainer<Component>().at(objectName);
        }

        template<typename Component>
        bool hasComponent(const std::string& objectName) const {
            auto it = objectMasks_.find(objectName);
            return it != objectMasks_.end() && it->second.test(getComponentId<Component>());
        }

        template<typename... QueryComponents>
        auto query() {
            std::vector<std::tuple<std::string, QueryComponents&...>> results;
            ComponentMask queryMask;
            (queryMask.set(getComponentId<QueryComponents>()), ...);
            for (auto& [objectName, objectMask]: objectMasks_) {
                if ((objectMask & queryMask) == queryMask) {
                    results.emplace_back(objectName, getComponent<QueryComponents>(objectName)...);
                }
            }
            return results;
        }

    private:
        template<typename Component>
        std::unordered_map<std::string, Component>& getComponentContainer() {
            return std::get<std::unordered_map<std::string, Component>>(componentContainers_);
        }

        template<typename Component>
        const std::unordered_map<std::string, Component>& getComponentContainer() const {
            return std::get<std::unordered_map<std::string, Component>>(componentContainers_);
        }

        static inline size_t nextComponentId = 0;
        template<typename T>
        static size_t getComponentId() {
            static size_t id = nextComponentId++;
            return id;
        }

        std::unordered_map<std::string, ComponentMask> objectMasks_;
        std::tuple<std::unordered_map<std::string, Components>...> componentContainers_;
    };
} // namespace detail

using ComponentStorage = detail::ComponentStorage<Transform, Camera, Renderer, BoxCollider>;
