#pragma once
#include <functional>
#include <log4cxx/logger.h>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

class AnimationClip;
class Mesh;
class Material;
class Model;
class Shader;
class Skeleton;
class Texture;

class AssetCache {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetCache");

public:
    AssetCache() {}

    template<typename T>
    T* get(const std::string& name) {
        auto& container = getContainer<T>();
        if (auto it = container.find(name); it != container.end()) return it->second.get();
        throw std::runtime_error("Asset not found: '" + name + "'");
    }

    template<typename T>
    T* insert(std::unique_ptr<T> asset) {
        auto assetName = asset->name;
        if (!asset) return nullptr;
        auto& container = getContainer<T>();
        auto [it, inserted] = container.emplace(assetName, std::move(asset));
        if (!inserted) {
            LOG4CXX_WARN(LOGGER, "Asset with name '" + assetName + "' already exists. Insertion skipped.");
            return nullptr;
        }
        return it->second.get();
    }

    // Containment is per type: package asset names can map to several assets of
    // different types (a .terrainpkg is both a Mesh and a Material).
    template<typename T>
    bool contains(const std::string& name) const {
        auto it = containers_.find(std::type_index(typeid(T)));
        if (it == containers_.end()) return false;
        return std::static_pointer_cast<Container<T>>(it->second)->contains(name);
    }

    template<typename T>
    void remove(const std::string& name) {
        getContainer<T>().erase(name);
    }

private:
    template<typename T>
    using Container = std::unordered_map<std::string, std::unique_ptr<T>>;

    template<typename T>
    Container<T>& getContainer() {
        auto type = std::type_index(typeid(T));
        auto it = containers_.find(type);
        if (it == containers_.end()) {
            auto container = std::make_shared<Container<T>>();
            containers_[type] = container;
            return *container;
        }
        return *std::static_pointer_cast<Container<T>>(it->second);
    }

    std::unordered_map<std::type_index, std::shared_ptr<void>> containers_;
};
