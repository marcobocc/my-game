#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include "core/assets/VirtualFileSystem.hpp"

class AssetBaker {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetBaker");

public:
    explicit AssetBaker(VirtualFileSystem& vfs) : vfs_(vfs) {}

    template<typename T>
    void bake(const T& asset, const std::string& assetName);

private:
    void makeParentDirs(const std::string& filePath) {
        auto parent = std::filesystem::path(filePath).parent_path().string();
        if (!parent.empty()) vfs_.makeDirs(parent);
    }

    VirtualFileSystem& vfs_;
};
