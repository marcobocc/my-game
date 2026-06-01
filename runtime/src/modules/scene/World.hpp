#pragma once
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "Actor.hpp"

class World {
public:
    Actor* addActor(EntityHandle handle) {
        if (handleToActor_.count(handle)) return nullptr;
        actors_.push_back(std::make_unique<Actor>(handle));
        Actor* ptr = actors_.back().get();
        handleToActor_[handle] = ptr;
        return ptr;
    }

    void removeActor(EntityHandle handle) {
        auto it = handleToActor_.find(handle);
        if (it == handleToActor_.end()) return;
        Actor* target = it->second;
        handleToActor_.erase(it);
        auto ait =
                std::ranges::find_if(actors_, [target](const std::unique_ptr<Actor>& a) { return a.get() == target; });
        if (ait != actors_.end()) actors_.erase(ait);
    }

    Actor* getActor(EntityHandle handle) {
        auto it = handleToActor_.find(handle);
        return it != handleToActor_.end() ? it->second : nullptr;
    }

    const Actor* getActor(EntityHandle handle) const {
        auto it = handleToActor_.find(handle);
        return it != handleToActor_.end() ? it->second : nullptr;
    }

    bool hasActor(EntityHandle handle) const { return handleToActor_.contains(handle); }

    void clear() {
        actors_.clear();
        handleToActor_.clear();
    }

    const std::vector<std::unique_ptr<Actor>>& getActors() const { return actors_; }

    template<typename... QueryComponents>
    auto query() const {
        std::vector<std::tuple<EntityHandle, const QueryComponents*...>> result;
        for (const auto& actor: actors_) {
            auto ptrs = std::make_tuple(
                    static_cast<const QueryComponents*>(actor->getComponent<QueryComponents>())...);
            if ((std::get<const QueryComponents*>(ptrs) && ...))
                result.emplace_back(actor->handle(), std::get<const QueryComponents*>(ptrs)...);
        }
        return result;
    }

private:
    std::vector<std::unique_ptr<Actor>> actors_;
    std::unordered_map<EntityHandle, Actor*> handleToActor_;
};
