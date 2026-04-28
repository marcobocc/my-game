#pragma once
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

class AssetStorage {
public:
    template<typename T>
    T* get(const std::string& name) {
        auto& container = getContainer<T>();
        if (auto it = container.find(name); it != container.end()) return it->second.get();
        return nullptr;
    }

    template<typename T>
    T* insert(const std::string& name, std::unique_ptr<T> asset) {
        if (!asset) return nullptr;
        auto& container = getContainer<T>();
        auto [it, inserted] = container.emplace(name, std::move(asset));
        names_.insert(name);
        return it->second.get();
    }

    bool contains(const std::string& name) const { return names_.contains(name); }

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
