#pragma once
#include <algorithm>
#include <filesystem>
#include <log4cxx/logger.h>
#include <unordered_map>
#include <vector>

class AssetExplorer {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetExplorer");

public:
    AssetExplorer() { discoverAssets(BUILTIN_ASSETS_DIR); }

    std::vector<std::string> discoverAssets(const std::filesystem::path& folder) {
        static constexpr std::array knownExtensions = {".shad", ".mesh", ".mat", ".model", ".jpg", ".png"};
        LOG4CXX_INFO(LOGGER, "Searching for assets in: " << folder);
        std::vector<std::string> discoveredAssets;
        for (const auto& file: std::filesystem::recursive_directory_iterator(folder)) {
            auto extension = file.path().extension().string();
            if (std::ranges::find(knownExtensions, extension) == knownExtensions.end()) continue;
            std::string assetName = std::filesystem::relative(file.path(), folder).string();
            registerAsset(assetName, file.path());
            discoveredAssets.push_back(assetName);
        }
        return discoveredAssets;
    }

    std::vector<std::string> list(const std::string& extensionFilter) const {
        std::vector<std::string> result;
        for (const auto& [name, path]: assets_) {
            if (path.extension() == extensionFilter) result.emplace_back(name);
        }
        return result;
    }

    void registerAsset(const std::string& assetName, const std::filesystem::path& absolutePath) {
        auto [it, inserted] = assets_.emplace(assetName, absolutePath);
        if (inserted) LOG4CXX_INFO(LOGGER, "Registered asset: " << assetName);
    }

    void deregisterAsset(const std::string& assetName) {
        if (assets_.erase(assetName)) LOG4CXX_INFO(LOGGER, "Deregistered asset: " << assetName);
    }

    std::filesystem::path getAbsolutePath(const std::string& assetName) const {
        auto it = assets_.find(assetName);
        if (it != assets_.end()) return it->second;
        return {};
    }

    static bool isBuiltin(const std::string& assetName) {
        auto filename = std::filesystem::path(assetName).filename().string();
        return std::ranges::starts_with(filename, "_");
    }

private:
    std::unordered_map<std::string, std::filesystem::path> assets_;
};
