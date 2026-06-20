#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "asset_types/AnimationClip.hpp"
#include "asset_types/Mesh.hpp"
#include "asset_types/Skeleton.hpp"

class VirtualFileSystem;

namespace importing {

    struct ImportedMeshFile {
        std::vector<std::unique_ptr<Mesh>> meshes;
        std::unique_ptr<Skeleton> skeleton; // nullptr if no bones
        std::vector<std::unique_ptr<AnimationClip>> clips;
    };

    ImportedMeshFile importMeshFile(const std::filesystem::path& filepath,
                                    bool reverseWinding,
                                    const std::string& namePrefix,
                                    const VirtualFileSystem& vfs);

} // namespace importing
