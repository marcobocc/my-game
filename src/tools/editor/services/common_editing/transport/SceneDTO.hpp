#pragma once
#include <nlohmann/json.hpp>
#include <vector>
#include "GameObjectDTO.hpp"

struct SceneDTO {
    std::vector<GameObjectDTO> objects;

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["objects"] = nlohmann::json::array();
        for (const auto& obj: objects) {
            j["objects"].push_back(obj.serialize());
        }
        return j;
    }

    static SceneDTO deserialize(const nlohmann::json& j) {
        SceneDTO scene;
        if (!j.contains("objects") || !j["objects"].is_array()) return scene;
        for (const auto& objJson: j["objects"]) {
            scene.objects.push_back(GameObjectDTO::deserialize(objJson));
        }
        return scene;
    }
};
