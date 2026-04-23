#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <string>
#include "../base/AssetLoader.hpp"
#include "assets/AssetCache.hpp"
#include "assets/types/AssetDescriptors.hpp"
#include "assets/types/mesh/Mesh.hpp"
#include "assets/types/mesh/details/ObjFileParser.hpp"

class MeshLoader : public AssetLoader {
    static constexpr std::string ASSET_TYPE_STR = "mesh";
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("MeshLoader");

public:
    bool load(const std::filesystem::path& assetDescriptorFile, AssetCache& dest) override {
        MeshDescriptor def = MeshDescriptor::fromFile(assetDescriptorFile);
        auto assetFolder = assetDescriptorFile.parent_path();
        auto meshFilePath = assetFolder / def.meshFile;
        if (meshFilePath.extension() == ".obj") {
            auto [vertices, indices] = ObjFileParser::parseFile(meshFilePath);
            if (!def.ccw) {
                for (size_t i = 0; i + 2 < indices.size(); i += 3)
                    std::swap(indices[i + 1], indices[i + 2]);
            }
            auto mesh = std::make_unique<Mesh>(def.name, vertices, indices);
            dest.insert<Mesh>(def.name, std::move(mesh));
            return true;
        }
        LOG4CXX_ERROR(LOGGER, "Unsupported mesh extension: " << meshFilePath.extension() << " in " << meshFilePath);
        return false;
    }

    std::string getAssetType() override { return ASSET_TYPE_STR; }
};
