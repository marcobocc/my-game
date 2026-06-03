#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include "GameObjectDTO.hpp"
#include "modules/scene/EntityHandle.hpp"

struct SceneDTO {
    std::vector<GameObjectDTO> objects;
    std::optional<EntityHandle> activeCameraHandle;

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["objects"] = nlohmann::json::array();
        for (const auto& obj: objects) {
            j["objects"].push_back(obj.serialize());
        }
        if (activeCameraHandle) j["activeCamera"] = *activeCameraHandle;
        return j;
    }

    static SceneDTO deserialize(const nlohmann::json& j) {
        SceneDTO scene;
        if (!j.contains("objects") || !j["objects"].is_array()) return scene;
        for (const auto& objJson: j["objects"]) {
            scene.objects.push_back(GameObjectDTO::deserialize(objJson));
        }
        if (j.contains("activeCamera") && !j["activeCamera"].is_null())
            scene.activeCameraHandle = j["activeCamera"].get<EntityHandle>();
        return scene;
    }
};
