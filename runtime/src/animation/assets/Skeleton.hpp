#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../core/assets/Asset.hpp"

struct Bone {
    std::string name;
    int parentIndex = -1;
    glm::mat4 offsetMatrix{1.0f}; // inverse bind-pose; identity for helper nodes
    glm::mat4 localBindTransform{1.0f}; // node-local transform in bind pose (from aiNode)
};

class Skeleton final : public Asset {
public:
    Skeleton(const std::string& name, std::vector<Bone> bones) : Asset(name), bones_(std::move(bones)) {
        for (int i = 0; i < static_cast<int>(bones_.size()); ++i)
            boneNameToIndex_[bones_[i].name] = i;
    }

    const std::vector<Bone>& getBones() const { return bones_; }
    size_t getBoneCount() const { return bones_.size(); }

    int getBoneIndex(const std::string& boneName) const {
        auto it = boneNameToIndex_.find(boneName);
        return it != boneNameToIndex_.end() ? it->second : -1;
    }

private:
    std::vector<Bone> bones_;
    std::unordered_map<std::string, int> boneNameToIndex_;
};
