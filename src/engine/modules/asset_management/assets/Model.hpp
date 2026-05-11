#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include "Asset.hpp"
#include "utils/JsonUtils.hpp"

class Model final : public Asset {
public:
    Model(const std::string& name, const std::string& meshName, const std::string& materialName) :
        Asset(name),
        meshName(meshName),
        materialName(materialName) {}

    static Model deserialize(const nlohmann::json& j, const std::filesystem::path& assetsRootPath) {
        auto name = assetsRootPath.filename().string();
        return {name,
                JsonUtils::getRequired<std::string>(j, "meshName"),
                JsonUtils::getRequired<std::string>(j, "materialName")};
    }

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["meshName"] = meshName;
        j["materialName"] = materialName;
        return j;
    }

    std::string meshName;
    std::string materialName;
};
