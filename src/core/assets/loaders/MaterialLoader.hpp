#pragma once
#include <memory>
#include <string>
#include "core/assets/types/Material.hpp"

class MaterialLoader {
public:
    std::unique_ptr<Material> load(const std::string& name) const {
        auto mat = std::make_unique<Material>();
        // TODO: Implement loading material from config
        return mat;
    }
};
