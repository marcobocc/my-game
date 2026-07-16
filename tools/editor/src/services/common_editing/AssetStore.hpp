#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "../UndoHistory.hpp"
#include "core/assets/AssetLoader.hpp"
#include "core/assets/PackagePaths.hpp"
#include "core/assets/VirtualFileSystem.hpp"
#include "details/AssetBaker.hpp"
#include "transport/GameObjectDTO.hpp"
#include "transport/SceneDTO.hpp"

class AssetStore {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetStore");

public:
    AssetStore(AssetLoader& loader, VirtualFileSystem& vfs, UndoHistory& undoHistory) :
        loader_(loader),
        vfs_(vfs),
        undoHistory_(undoHistory),
        baker_(vfs) {}

    void saveChanges() {
        for (const auto& bakeFunc: bakeList_ | std::views::values) {
            if (bakeFunc) bakeFunc();
        }
        bakeList_.clear();
    }

    // --------------------------------------------------------------------------------
    // Queries
    // --------------------------------------------------------------------------------
    template<typename T>
    const T& getAsset(const std::string& assetName) const {
        auto asset = loader_.get<T>(assetName);
        if (!asset) throw std::runtime_error("Asset '" + assetName + "'does not exist");
        return *asset;
    }

    std::optional<SceneDTO> readPrefab(const std::string& assetName) const {
        if (!vfs_.exists(assetName)) return std::nullopt;
        try {
            auto data = vfs_.read(assetName);
            auto j = nlohmann::json::parse(data.begin(), data.end());
            return SceneDTO::deserialize(j);
        } catch (...) {
            return std::nullopt;
        }
    }

    // Files inside bundle packages (.matpkg/.modelpkg) are collapsed into a single
    // entry: the package path itself.
    std::vector<std::string> listAll() const {
        std::vector<std::string> result;
        std::unordered_set<std::string> seenPackages;
        for (const auto& file: vfs_.listFilesRecursive()) {
            if (auto pkg = PackagePaths::enclosingPackage(file)) {
                if (seenPackages.insert(*pkg).second) result.emplace_back(*pkg);
            } else {
                result.emplace_back(file);
            }
        }
        return result;
    }

    // Also returns bundle packages whose primary asset matches the filter
    // (".mat" includes .matpkg packages, ".model" includes .modelpkg).
    std::vector<std::string> list(const std::string& extensionFilter) const {
        auto packageExt = PackagePaths::packageExtensionFor(extensionFilter);
        std::vector<std::string> result;
        for (const auto& file: listAll()) {
            auto ext = std::filesystem::path(file).extension();
            if (ext == extensionFilter || (packageExt && ext == *packageExt)) {
                result.emplace_back(file);
            }
        }
        return result;
    }

    static bool isBuiltin(const std::string& assetName) {
        return std::string_view(assetName).starts_with("assets/internal");
    }

    // Inner package files are only editable through their package (the package path is
    // the asset name everywhere; AssetBaker resolves it to the inner primary file).
    // Exception: terrain splatmaps are painted in place by the terrain tools, which
    // reach them via the material's real inner texture path.
    static bool isMutable(const std::string& assetName) {
        if (isBuiltin(assetName)) return false;
        if (!PackagePaths::isInsidePackage(assetName)) return true;
        return assetName.ends_with(".splatmap.png");
    }

    bool exists(const std::string& assetName) const { return vfs_.exists(assetName); }

    // --------------------------------------------------------------------------------
    // Mutations
    // --------------------------------------------------------------------------------

    template<typename T>
    void createAssetFile(const T& asset, const std::string& mutationGroup = "") {
        baker_.bake<T>(asset, asset.name);
        // For packages the baked file is the inner primary file, not the package dir.
        std::string filePath = PackagePaths::resolveAssetFile(asset.name, vfs_);
        std::vector<uint8_t> fileBlobBackup = vfs_.read(filePath);
        undoHistory_.push([this, asset] { destroyAssetFile_Impl(asset.name); },
                          [this, filePath, fileBlobBackup] { restoreAssetFile_Impl(filePath, fileBlobBackup); },
                          "Create Asset",
                          mutationGroup);
    }

    void destroyAssetFile(const std::string& assetName, const std::string& mutationGroup = "") {
        if (!isMutable(assetName)) return;
        if (!exists(assetName)) return;
        // Packages are directories: back up every inner file so undo can restore the bundle.
        std::vector<std::pair<std::string, std::vector<unsigned char>>> backup;
        if (PackagePaths::isPackage(assetName)) {
            for (const auto& file: vfs_.listFilesRecursive(assetName))
                backup.emplace_back(file, vfs_.read(file));
        } else {
            backup.emplace_back(assetName, vfs_.read(assetName));
        }
        destroyAssetFile_Impl(assetName);
        undoHistory_.push(
                [this, backup] {
                    for (const auto& [path, blob]: backup)
                        restoreAssetFile_Impl(path, blob);
                },
                [this, assetName] { destroyAssetFile_Impl(assetName); },
                "Delete Asset",
                mutationGroup);
    }

    template<typename T, typename Fn>
    void mutateAsset(const std::string& assetName, Fn&& mutation, const std::string& mutationGroup = "") {
        if (!isMutable(assetName)) return;
        if (!exists(assetName)) return;
        T backup = getAsset<T>(assetName);
        T* ptr = loader_.get<T>(assetName);
        mutateAsset_Impl<T>(ptr, assetName, std::forward<Fn>(mutation));
        T after = getAsset<T>(assetName);
        undoHistory_.push(
                [this, ptr, assetName, backup] {
                    mutateAsset_Impl<T>(ptr, assetName, [&backup](T& a) { a = backup; });
                },
                [this, ptr, assetName, after] { mutateAsset_Impl<T>(ptr, assetName, [&after](T& a) { a = after; }); },
                "Modify Asset",
                mutationGroup);
    }

    AssetLoader& loader() { return loader_; }

    // Fires after any in-memory asset mutation (including undo/redo).
    void setOnAssetMutated(std::function<void(const std::string&)> callback) { onAssetMutated_ = std::move(callback); }

private:
    AssetLoader& loader_;
    VirtualFileSystem& vfs_;
    UndoHistory& undoHistory_;
    AssetBaker baker_;
    std::unordered_map<std::string, std::function<void()>> bakeList_;

    template<typename T>
    void createAssetFile_Impl(const T& asset) {
        baker_.bake<T>(asset, asset.name);
    }

    void destroyAssetFile_Impl(const std::string& assetName) {
        LOG4CXX_INFO(LOGGER, "Deleting " << assetName);
        if (auto it = bakeList_.find(assetName); it != bakeList_.end()) {
            it->second();
            bakeList_.erase(it);
        }
        vfs_.removeRecursive(assetName);
    }

    void restoreAssetFile_Impl(const std::string& assetName, const std::vector<unsigned char>& fileBlob) {
        LOG4CXX_INFO(LOGGER, "Restoring " << assetName);
        auto parent = std::filesystem::path(assetName).parent_path().string();
        if (!parent.empty()) vfs_.makeDirs(parent);
        vfs_.write(assetName, fileBlob);
    }

    template<typename T, typename Fn>
    void mutateAsset_Impl(T* asset, const std::string& assetName, Fn&& mutation) {
        std::forward<Fn>(mutation)(*asset);
        bakeList_[assetName] = [this, assetName] { baker_.bake<T>(getAsset<T>(assetName), assetName); };
        if (onAssetMutated_) onAssetMutated_(assetName);
    }

    std::function<void(const std::string&)> onAssetMutated_;
};
