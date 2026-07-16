#include <gtest/gtest.h>
#include "core/assets/PackagePaths.hpp"

TEST(PackagePaths, IsPackage) {
    EXPECT_TRUE(PackagePaths::isPackage("assets/props/barrel.modelpkg"));
    EXPECT_TRUE(PackagePaths::isPackage("assets/props/rust.matpkg"));
    EXPECT_TRUE(PackagePaths::isPackage("Terrain.terrainpkg"));
    EXPECT_FALSE(PackagePaths::isPackage("assets/props/barrel.model"));
    EXPECT_FALSE(PackagePaths::isPackage("assets/props/barrel.mat"));
    EXPECT_FALSE(PackagePaths::isPackage("assets/props"));
}

TEST(PackagePaths, PrimaryExtension) {
    EXPECT_EQ(PackagePaths::primaryExtension("a/b.matpkg"), ".mat");
    EXPECT_EQ(PackagePaths::primaryExtension("a/b.modelpkg"), ".model");
    EXPECT_EQ(PackagePaths::primaryExtension("a/b.terrainpkg"), ".mesh");
    EXPECT_EQ(PackagePaths::primaryExtension("a/b.mat"), std::nullopt);
}

TEST(PackagePaths, DefaultInnerPath) {
    EXPECT_EQ(PackagePaths::defaultInnerPath("a/Terrain.terrainpkg", ".mat"), "a/Terrain.terrainpkg/Terrain.mat");
    EXPECT_EQ(PackagePaths::defaultInnerPath("a/Terrain.terrainpkg", ".splatmap.png"),
              "a/Terrain.terrainpkg/Terrain.splatmap.png");
    EXPECT_EQ(PackagePaths::defaultPrimaryPath("a/barrel.modelpkg"), "a/barrel.modelpkg/barrel.model");
    EXPECT_EQ(PackagePaths::defaultPrimaryPath("a/Terrain.terrainpkg"), "a/Terrain.terrainpkg/Terrain.mesh");
}

TEST(PackagePaths, EnclosingPackage) {
    EXPECT_EQ(PackagePaths::enclosingPackage("assets/props/barrel.modelpkg/barrel.mesh"),
              "assets/props/barrel.modelpkg");
    EXPECT_EQ(PackagePaths::enclosingPackage("assets/props/rust.matpkg/albedo.png"), "assets/props/rust.matpkg");
    EXPECT_EQ(PackagePaths::enclosingPackage("assets/props/barrel.mat"), std::nullopt);
    EXPECT_EQ(PackagePaths::enclosingPackage("barrel.mat"), std::nullopt);
    // The package dir itself is not "inside" a package.
    EXPECT_EQ(PackagePaths::enclosingPackage("assets/props/barrel.modelpkg"), std::nullopt);
}
