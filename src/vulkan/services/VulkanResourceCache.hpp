#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

template<typename Resource>
class VulkanResourceCache {
public:
    template<typename... Args>
    Resource* createOrGet(const std::string& name, Args&&... args) {
        auto it = resources_.find(name);
        if (it != resources_.end()) return it->second.get();

        auto resource = std::make_unique<Resource>(std::forward<Args>(args)...);
        auto [insertedIt, _] = resources_.emplace(name, std::move(resource));
        return insertedIt->second.get();
    }


    Resource* get(const std::string& name) {
        auto it = resources_.find(name);
        return (it != resources_.end()) ? it->second.get() : nullptr;
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Resource>> resources_;
};
