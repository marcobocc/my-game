#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include "GameObjectDTO.hpp"
#include "modules/scene/EntityHandle.hpp"

struct HierarchyNode {
    EntityHandle handle{};
    std::vector<HierarchyNode> children;

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["handle"] = handle;
        j["children"] = nlohmann::json::array();
        for (const auto& child: children)
            j["children"].push_back(child.serialize());
        return j;
    }

    static HierarchyNode deserialize(const nlohmann::json& j) {
        HierarchyNode node;
        node.handle = j.at("handle").get<EntityHandle>();
        if (j.contains("children") && j["children"].is_array()) {
            for (const auto& c: j["children"])
                node.children.push_back(HierarchyNode::deserialize(c));
        }
        return node;
    }
};

struct SceneDTO {
    std::vector<GameObjectDTO> objects;
    std::vector<HierarchyNode> hierarchy;
    std::optional<EntityHandle> activeCameraHandle;

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["objects"] = nlohmann::json::array();
        for (const auto& obj: objects)
            j["objects"].push_back(obj.serialize());
        j["hierarchy"] = nlohmann::json::array();
        for (const auto& node: hierarchy)
            j["hierarchy"].push_back(node.serialize());
        if (activeCameraHandle) j["activeCamera"] = *activeCameraHandle;
        return j;
    }

    static SceneDTO deserialize(const nlohmann::json& j) {
        SceneDTO scene;
        if (!j.contains("objects") || !j["objects"].is_array()) return scene;
        for (const auto& objJson: j["objects"])
            scene.objects.push_back(GameObjectDTO::deserialize(objJson));
        if (j.contains("hierarchy") && j["hierarchy"].is_array()) {
            for (const auto& nodeJson: j["hierarchy"])
                scene.hierarchy.push_back(HierarchyNode::deserialize(nodeJson));
        }
        if (j.contains("activeCamera") && !j["activeCamera"].is_null())
            scene.activeCameraHandle = j["activeCamera"].get<EntityHandle>();
        return scene;
    }
};
