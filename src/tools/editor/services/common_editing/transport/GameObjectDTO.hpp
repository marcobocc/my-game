#pragma once
#include "../../../../../runtime/modules/scene/EntityStore.hpp"

struct GameObjectDTO {
    using ComponentVariant = std::variant<Transform, Camera, Renderer, BoxCollider, Light, Metadata>;
    EntityHandle handle;
    std::vector<ComponentVariant> components;

    explicit GameObjectDTO(EntityHandle handle) : handle(handle) {}

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["guid"] = handle;
        j["components"] = nlohmann::json::array();
        for (const auto& c: components) {
            std::visit(
                    [&]<typename T0>(const T0& comp) {
                        using T = std::decay_t<T0>;
                        nlohmann::json entry;
                        entry["type"] = comp.typeName();
                        entry["data"] = comp.serialize();
                        j["components"].push_back(entry);
                    },
                    c);
        }
        return j;
    }

    static GameObjectDTO deserialize(const nlohmann::json& j) {
        GameObjectDTO dto(j.value("guid", 0));
        for (const auto& c: j["components"]) {
            std::string type = c["type"];
            if (type == "Transform")
                dto.components.emplace_back(Transform::deserialize(c["data"]));
            else if (type == "Camera")
                dto.components.emplace_back(Camera::deserialize(c["data"]));
            else if (type == "Renderer")
                dto.components.emplace_back(Renderer::deserialize(c["data"]));
            else if (type == "BoxCollider")
                dto.components.emplace_back(BoxCollider::deserialize(c["data"]));
            else if (type == "Light")
                dto.components.emplace_back(Light::deserialize(c["data"]));
            else if (type == "Metadata")
                dto.components.emplace_back(Metadata::deserialize(c["data"]));
            else
                throw std::runtime_error("Invalid component: " + type);
        }
        return dto;
    }
};
