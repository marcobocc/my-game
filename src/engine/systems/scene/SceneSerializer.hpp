#pragma once
#include <nlohmann/json.hpp>
#include "systems/scene/SceneManager.hpp"

namespace SceneSerializer {
    nlohmann::json serializeScene(const SceneManager& scene);
    void deserializeScene(const nlohmann::json& j, SceneManager& scene);
} // namespace SceneSerializer
