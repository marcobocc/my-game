#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include "Asset.hpp"

struct KeyframeVec3 {
    float time;
    glm::vec3 value;
};

struct KeyframeQuat {
    float time;
    glm::quat value;
};

struct BoneTrack {
    int boneIndex = -1;
    std::vector<KeyframeVec3> positionKeys;
    std::vector<KeyframeQuat> rotationKeys;
    std::vector<KeyframeVec3> scaleKeys;
};

class AnimationClip final : public Asset {
public:
    AnimationClip(const std::string& name, float duration, std::vector<BoneTrack> tracks) :
        Asset(name),
        duration_(duration),
        tracks_(std::move(tracks)) {}

    float getDuration() const { return duration_; }
    const std::vector<BoneTrack>& getTracks() const { return tracks_; }

private:
    float duration_ = 0.0f;
    std::vector<BoneTrack> tracks_;
};
