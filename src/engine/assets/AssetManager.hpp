#pragma once
#include <string>
#include "AssetImporter.hpp"
#include "AssetStorage.hpp"

class AssetManager {
public:
    explicit AssetManager(AssetImporter& importer, AssetStorage& cache) : assetImporter_(importer), storage_(cache) {}

    template<typename T>
    T* get(const std::string& name) {
        if (auto* cached = storage_.get<T>(name)) return cached;
        bool importSuccessful = assetImporter_.import(name);
        if (!importSuccessful) throw std::runtime_error("Could not load asset: " + name);
        return storage_.get<T>(name);
    }

private:
    AssetImporter& assetImporter_;
    AssetStorage& storage_;
};
