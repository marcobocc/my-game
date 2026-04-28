#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "assets/types/base/Asset.hpp"

class Mesh final : public Asset {
public:
    Mesh(const std::string& name,
         std::vector<glm::vec3> positions,
         std::vector<glm::vec2> uvs,
         std::vector<glm::vec3> colors,
         std::vector<uint32_t> indices) :
        Asset(name),
        positions_(std::move(positions)),
        uvs_(std::move(uvs)),
        colors_(std::move(colors)),
        indices_(std::move(indices)) {}

    bool hasIndices() const { return !indices_.empty(); }
    bool hasUvs() const { return !uvs_.empty(); }
    bool hasColors() const { return !colors_.empty(); }
    size_t getVertexCount() const { return positions_.size(); }

    const std::vector<glm::vec3>& getPositions() const { return positions_; }
    const std::vector<glm::vec2>& getUvs() const { return uvs_; }
    const std::vector<glm::vec3>& getColors() const { return colors_; }
    const std::vector<uint32_t>& getIndices() const { return indices_; }

private:
    std::vector<glm::vec3> positions_;
    std::vector<glm::vec2> uvs_;
    std::vector<glm::vec3> colors_;
    std::vector<uint32_t> indices_;
};
