#pragma once
#include <nlohmann/json.hpp>
#include "core/scene/Scene.hpp"

namespace SceneSerializer {
    nlohmann::json serializeScene(const Scene& scene);
    void deserializeScene(const nlohmann::json& j, Scene& scene);
} // namespace SceneSerializer
