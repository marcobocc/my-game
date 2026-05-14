#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include "../UndoHistory.hpp"
#include "AssetBaker.hpp"
#include "modules/asset_management/AssetCache.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/VirtualFileSystem.hpp"

class EditorAssetRepository {
private:
    template<typename T>
    struct Snapshot {
        std::optional<T> before;
        std::optional<T> after;
        bool dirty = false;
    };

public:
    EditorAssetRepository(AssetLoader& loader, VirtualFileSystem& vfs, AssetCache& cache, UndoHistory& undoHistory) :
        loader_(loader),
        vfs_(vfs),
        cache_(cache),
        undoHistory_(undoHistory),
        baker_(vfs) {}

    template<typename T>
    const T* get(const std::string& assetName) const {
        return loader_.get<T>(assetName);
    }

    template<typename T>
    T* get(const std::string& assetName) {
        return loader_.get<T>(assetName);
    }

    template<typename T>
    void insert(const std::string& assetName, const T& asset) {
        if (!isMutable(assetName)) return;
        baker_.bake<T>(asset, assetName);
        cache_.insert<T>(assetName, std::make_unique<T>(asset));
        snapshots<T>()[assetName] = Snapshot<T>{.before = std::nullopt, .after = asset, .dirty = false};
        undoHistory_.push([this, assetName] { remove<T>(assetName); },
                          [this, assetName, asset] { insert<T>(assetName, asset); });
    }

    template<typename T>
    void update(const std::string& assetName, const T& updatedAsset) {
        if (!isMutable(assetName)) return;
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
        if (!isMutable(assetName)) return;
        T* asset = get<T>(assetName);
        if (!asset) return;
        auto& snapshot = snapshots<T>()[assetName];
        if (!snapshot.before) {
            snapshot.before = *asset;
        }
        vfs_.remove(assetName);
        snapshot.after = std::nullopt;
        snapshot.dirty = false;
        undoHistory_.push(
                [this, assetName] {
                    auto& s = snapshots<T>()[assetName];
                    if (s.before) {
                        insert<T>(assetName, *s.before);
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
            baker_.bake<T>(*snapshot.after, assetName);
            snapshot.dirty = false;
        }
    }

    std::vector<std::string> listAll() const { return vfs_.listFilesRecursive(); }

    std::vector<std::string> list(const std::string& extensionFilter) const {
        std::vector<std::string> result;
        for (const auto& file: vfs_.listFilesRecursive()) {
            if (std::filesystem::path(file).extension() == extensionFilter) {
                result.emplace_back(file);
            }
        }
        return result;
    }

    static bool isBuiltin(const std::string& assetName) {
        return std::string_view(assetName).starts_with("assets/builtin/");
    }

    static bool isMutable(const std::string& assetName) { return !isBuiltin(assetName); }

    bool exists(const std::string& assetName) const { return vfs_.exists(assetName); }

private:
    AssetLoader& loader_;
    VirtualFileSystem& vfs_;
    AssetCache& cache_;
    UndoHistory& undoHistory_;
    AssetBaker baker_;

    template<typename T>
    static auto& snapshots() {
        static std::unordered_map<std::string, Snapshot<T>> storage;
        return storage;
    }
};
