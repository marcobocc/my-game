#pragma once
#include <string>
#include "Asset.hpp"

class Model final : public Asset {
public:
    Model(const std::string& name, const std::string& meshName, const std::string& materialName) :
        Asset(name),
        meshName_(meshName),
        materialName_(materialName) {}

    const std::string& getMeshName() const { return meshName_; }
    const std::string& getMaterialName() const { return materialName_; }

private:
    std::string meshName_;
    std::string materialName_;
};
