#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include "VirtualFileSystem.hpp"

/*
    Bundle assets ("packages") are directories with a package extension:
      - foo.matpkg      contains exactly one .mat (+ optional private textures)
      - foo.modelpkg    contains exactly one .model (+ optional private .mesh/.mat/textures/sources)
      - foo.terrainpkg  contains one .mesh (+ its .obj), one terrain-blend .mat and its splatmap
                        texture; the package name resolves per requested type (Mesh -> .mesh,
                        Material -> .mat)

    The package path is the asset name used everywhere outside the package (editor,
    references, thumbnails); the loader resolves it to the inner primary file.
    Packages do not nest.
*/
namespace PackagePaths {

    inline constexpr std::string_view MATERIAL_PACKAGE_EXT = ".matpkg";
    inline constexpr std::string_view MODEL_PACKAGE_EXT = ".modelpkg";
    inline constexpr std::string_view TERRAIN_PACKAGE_EXT = ".terrainpkg";

    inline bool isPackage(const std::string& path) {
        auto ext = std::filesystem::path(path).extension().string();
        return ext == MATERIAL_PACKAGE_EXT || ext == MODEL_PACKAGE_EXT || ext == TERRAIN_PACKAGE_EXT;
    }

    // Extension of the package's defining asset file (for terrains: the mesh).
    inline std::optional<std::string> primaryExtension(const std::string& packagePath) {
        auto ext = std::filesystem::path(packagePath).extension().string();
        if (ext == MATERIAL_PACKAGE_EXT) return ".mat";
        if (ext == MODEL_PACKAGE_EXT) return ".model";
        if (ext == TERRAIN_PACKAGE_EXT) return ".mesh";
        return std::nullopt;
    }

    // Package extension bundling a given primary asset extension (".mat" -> ".matpkg").
    inline std::optional<std::string> packageExtensionFor(const std::string& primaryExt) {
        if (primaryExt == ".mat") return std::string(MATERIAL_PACKAGE_EXT);
        if (primaryExt == ".model") return std::string(MODEL_PACKAGE_EXT);
        if (primaryExt == ".mesh") return std::string(TERRAIN_PACKAGE_EXT);
        return std::nullopt;
    }

    // Nearest ancestor directory with a package extension, if any.
    inline std::optional<std::string> enclosingPackage(const std::string& path) {
        std::string current = path;
        while (true) {
            auto pos = current.find_last_of('/');
            if (pos == std::string::npos) return std::nullopt;
            current = current.substr(0, pos);
            if (isPackage(current)) return current;
        }
    }

    inline bool isInsidePackage(const std::string& path) { return enclosingPackage(path).has_value(); }

    // Package directory -> inner asset path with the given extension, if present.
    inline std::optional<std::string>
    resolveInner(const std::string& packagePath, const VirtualFileSystem& vfs, const std::string& innerExt) {
        for (const auto& file: vfs.listFiles(packagePath)) {
            if (std::filesystem::path(file).extension() == innerExt) return packagePath + "/" + file;
        }
        return std::nullopt;
    }

    // Package directory -> inner primary asset path (e.g. ".../barrel.modelpkg/barrel.model").
    inline std::optional<std::string> resolvePrimary(const std::string& packagePath, const VirtualFileSystem& vfs) {
        auto primaryExt = primaryExtension(packagePath);
        if (!primaryExt) return std::nullopt;
        return resolveInner(packagePath, vfs, *primaryExt);
    }

    // Conventional inner path for a file that may not exist yet
    // (e.g. "a/barrel.modelpkg" + ".model" -> "a/barrel.modelpkg/barrel.model").
    inline std::string defaultInnerPath(const std::string& packagePath, const std::string& innerExt) {
        auto stem = std::filesystem::path(packagePath).stem().string();
        return packagePath + "/" + stem + innerExt;
    }

    inline std::string defaultPrimaryPath(const std::string& packagePath) {
        return defaultInnerPath(packagePath, primaryExtension(packagePath).value_or(""));
    }

    // For any asset name: if it's a package, its primary file; otherwise the name itself.
    inline std::string resolveAssetFile(const std::string& name, const VirtualFileSystem& vfs) {
        if (!isPackage(name)) return name;
        return resolvePrimary(name, vfs).value_or(defaultPrimaryPath(name));
    }

    // Type-aware variant: a package name resolves to the inner file of the requested
    // extension (terrain packages hold several loadable assets: .mesh and .mat).
    inline std::string
    resolveAssetFile(const std::string& name, const VirtualFileSystem& vfs, const std::string& innerExt) {
        if (!isPackage(name)) return name;
        return resolveInner(name, vfs, innerExt).value_or(defaultInnerPath(name, innerExt));
    }

} // namespace PackagePaths
