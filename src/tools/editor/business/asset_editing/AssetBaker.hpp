#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <memory>

class AssetBaker {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetBaker");

public:
    template<typename T>
    static void bake(const T& asset, const std::string& assetName, const std::filesystem::path& outputPath);

private:
    static void ensureDirectory(const std::filesystem::path& path);
};
