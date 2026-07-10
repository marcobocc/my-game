#pragma once
#include <filesystem>
#include "../../core/assets/Asset.hpp"

struct LuaScript : Asset {
    explicit LuaScript(const std::string& name, std::filesystem::path path) : Asset(name), path(std::move(path)) {}

    std::filesystem::path path;
};
