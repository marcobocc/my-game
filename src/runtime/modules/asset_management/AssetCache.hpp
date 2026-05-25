#pragma once
#include <functional>
#include <log4cxx/logger.h>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

class Mesh;
class Texture;
class Shader;
class Material;
class Model;

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
        names_.insert(assetName);
        return it->second.get();
    }

    bool contains(const std::string& name) const { return names_.contains(name); }

    template<typename T>
    void remove(const std::string& name) {
        auto& container = getContainer<T>();
        container.erase(name);
        names_.erase(name);
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
    std::unordered_set<std::string> names_;
};
