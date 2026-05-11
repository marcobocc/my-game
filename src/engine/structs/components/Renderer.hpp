#pragma once
#include <glm/vec4.hpp>
#include <optional>
#include <string>
#include "../../utils/JsonUtils.hpp"

struct Renderer {
    std::string meshName;
    std::string materialName;
    std::optional<glm::vec4> tintOverride;
    bool enabled = true;

    nlohmann::json serialize() const {
        nlohmann::json j = {{"meshName", meshName}, {"materialName", materialName}, {"enabled", enabled}};
        if (tintOverride.has_value()) {
            const auto& c = *tintOverride;
            j["tintOverride"] = {c.r, c.g, c.b, c.a};
        }
        return j;
    }

    static Renderer deserialize(const nlohmann::json& j) {
        Renderer r{};
        r.meshName = j.at("meshName").get<std::string>();
        r.materialName = j.at("materialName").get<std::string>();
        r.enabled = j.at("enabled").get<bool>();
        if (j.contains("tintOverride")) {
            auto a = j.at("tintOverride").get<std::array<float, 4>>();
            r.tintOverride = glm::vec4(a[0], a[1], a[2], a[3]);
        }
        return r;
    }
};
