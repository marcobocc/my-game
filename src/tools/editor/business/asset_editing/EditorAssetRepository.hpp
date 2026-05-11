#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include "../../../../engine/modules/asset_management/AssetCache.hpp"
#include "../../../../engine/modules/asset_management/AssetExplorer.hpp"
#include "../../../../engine/modules/asset_management/AssetLoader.hpp"
#include "../UndoHistory.hpp"

class EditorAssetRepository {
private:
    template<typename T>
    struct Snapshot {
        std::optional<T> before;
        std::optional<T> after;
        bool dirty = false;
    };

public:
    EditorAssetRepository(AssetLoader& loader,
                          AssetExplorer& explorer,
                          AssetCache& cache,
                          UndoHistory& undoHistory,
                          std::filesystem::path projectFolder) :
        loader_(loader),
        explorer_(explorer),
        cache_(cache),
        undoHistory_(undoHistory),
        projectFolder_(std::move(projectFolder)) {}

    template<typename T>
    const T* get(const std::string& assetName) const {
        return loader_.get<T>(assetName);
    }

    template<typename T>
    T* get(const std::string& assetName) {
        return loader_.get<T>(assetName);
    }

    template<typename T>
    void create(const std::string& assetName, const T& asset) {
        std::filesystem::path fullPath = explorer_.getAbsolutePath(assetName);
        if (fullPath.empty()) fullPath = projectFolder_ / assetName;
        saveAsset(fullPath, asset);
        explorer_.registerAsset(assetName, fullPath);
        cache_.insert<T>(assetName, std::make_unique<T>(asset));
        snapshots<T>()[assetName] = Snapshot<T>{.before = std::nullopt, .after = asset, .dirty = false};
        undoHistory_.push([this, assetName] { remove<T>(assetName); },
                          [this, assetName, asset] { create<T>(assetName, asset); });
    }

    template<typename T>
    void update(const std::string& assetName, const T& updatedAsset) {
        T* asset = get<T>(assetName);
        if (!asset) return;
        auto& snapshot = snapshots<T>()[assetName];
        if (!snapshot.before) {
            snapshot.before = *asset;
        }
        snapshot.after = updatedAsset;
        snapshot.dirty = true;
        *asset = updatedAsset;
        undoHistory_.push(
                [this, assetName] {
                    auto& s = snapshots<T>()[assetName];
                    if (s.before) {
                        update<T>(assetName, *s.before);
                    }
                },
                [this, assetName, updatedAsset] { update<T>(assetName, updatedAsset); });
    }

    template<typename T, typename Fn>
    void mutate(const std::string& assetName, Fn&& mutation) {
        if (!isMutable(assetName)) return;
        T* asset = get<T>(assetName);
        if (!asset) return;
        T updated = *asset;
        mutation(updated);
        update<T>(assetName, updated);
    }

    template<typename T>
    void remove(const std::string& assetName) {
        T* asset = get<T>(assetName);
        if (!asset) return;
        auto& snapshot = snapshots<T>()[assetName];
        if (!snapshot.before) {
            snapshot.before = *asset;
        }
        std::filesystem::path fullPath = explorer_.getAbsolutePath(assetName);
        std::filesystem::remove(fullPath);
        explorer_.deregisterAsset(assetName);
        snapshot.after = std::nullopt;
        snapshot.dirty = false;
        undoHistory_.push(
                [this, assetName] {
                    auto& s = snapshots<T>()[assetName];
                    if (s.before) {
                        create<T>(assetName, *s.before);
                    }
                },
                [this, assetName] { remove<T>(assetName); });
    }

    template<typename T>
    void commit() {
        auto& typedSnapshots = snapshots<T>();
        for (auto& [assetName, snapshot]: typedSnapshots) {
            if (!snapshot.dirty) continue;
            if (!snapshot.after) continue;
            std::filesystem::path fullPath = explorer_.getAbsolutePath(assetName);
            saveAsset(fullPath, *snapshot.after);
            snapshot.dirty = false;
        }
    }

    std::vector<std::string> list(const std::string& extensionFilter) const { return explorer_.list(extensionFilter); }

    bool isMutable(const std::string& assetName) const { return !explorer_.isBuiltin(assetName); }

private:
    AssetLoader& loader_;
    AssetExplorer& explorer_;
    AssetCache& cache_;
    UndoHistory& undoHistory_;
    std::filesystem::path projectFolder_;

    template<typename T>
    static auto& snapshots() {
        static std::unordered_map<std::string, Snapshot<T>> storage;
        return storage;
    }

    template<typename T>
    static void saveAsset(const std::filesystem::path& fullPath, const T& asset) {
        std::ofstream file(fullPath);
        if (!file.is_open()) throw std::runtime_error("Failed to open asset file for writing: " + fullPath.string());
        file << asset.serialize().dump(4);
    }
};
