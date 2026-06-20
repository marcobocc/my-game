#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "modules/asset_management/AssetCache.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/asset_types/AnimationClip.hpp"
#include "modules/asset_management/asset_types/Mesh.hpp"
#include "modules/asset_management/asset_types/Skeleton.hpp"
#include "modules/scene/World.hpp"
#include "modules/scene/components/Animator.hpp"
#include "modules/scene/components/Renderer.hpp"

// Maximum bones per skeleton supported by the GPU skinning UBO.
inline constexpr uint32_t MAX_BONES = 256;

// Per-entity skinning matrices uploaded to the GPU each frame.
struct SkinningMatrices {
    glm::mat4 bones[MAX_BONES];
};

class AnimationSystem {
public:
    explicit AnimationSystem(AssetLoader& loader) : loader_(loader) {}

    void update(float dt, World& world) {
        auto animators = world.query<Animator, Renderer>();
        for (auto& [entity, animatorPtr, rendererPtr]: animators) {
            if (!animatorPtr) continue;
            Animator& anim = *animatorPtr;

            // Auto-discover clip names from the mesh when the component has none.
            if (anim.clipNames.empty() && rendererPtr) {
                const Mesh* mesh = loader_.get<Mesh>(rendererPtr->meshName);
                if (mesh && !mesh->getClipNames().empty()) {
                    anim.clipNames = mesh->getClipNames();
                    if (anim.currentClip.empty()) anim.currentClip = anim.clipNames[0];
                }
            }

            if (!anim.playing) continue;

            // Derive skeleton from the mesh.
            const Skeleton* skeleton = nullptr;
            if (rendererPtr) {
                const Mesh* mesh = loader_.get<Mesh>(rendererPtr->meshName);
                if (mesh && !mesh->getSkeletonName().empty()) skeleton = loader_.get<Skeleton>(mesh->getSkeletonName());
            }
            if (!skeleton) continue;

            if (anim.currentClip.empty()) continue;
            AnimationClip* clip = loader_.get<AnimationClip>(anim.currentClip);
            if (!clip) continue;

            anim.currentTime += dt * anim.speed;
            if (anim.loop) {
                if (clip->getDuration() > 0.0f) anim.currentTime = std::fmod(anim.currentTime, clip->getDuration());
            } else {
                anim.currentTime = std::min(anim.currentTime, clip->getDuration());
                if (anim.currentTime >= clip->getDuration()) anim.playing = false;
            }

            skinningCache_[entity] = computeSkinning(*skeleton, *clip, anim.currentTime);
        }
    }

    // Returns the skinning matrices for the given entity, or identity matrices if none.
    const SkinningMatrices& getSkinningMatrices(EntityHandle entity) const {
        auto it = skinningCache_.find(entity);
        if (it != skinningCache_.end()) return it->second;
        return identity_;
    }

    bool hasSkinning(EntityHandle entity) const { return skinningCache_.count(entity) > 0; }

private:
    static int findKeyBefore(float time, size_t count, auto getTime) {
        int idx = 0;
        for (int i = 0; i < static_cast<int>(count) - 1; ++i) {
            if (getTime(i + 1) > time) break;
            idx = i + 1;
        }
        return idx;
    }

    static glm::vec3 sampleVec3(const std::vector<KeyframeVec3>& keys, float time) {
        if (keys.empty()) return glm::vec3(0.0f);
        if (keys.size() == 1) return keys[0].value;
        int i = findKeyBefore(time, keys.size(), [&](int k) { return keys[k].time; });
        int j = std::min(i + 1, static_cast<int>(keys.size()) - 1);
        if (i == j) return keys[i].value;
        float t = (time - keys[i].time) / (keys[j].time - keys[i].time);
        return glm::mix(keys[i].value, keys[j].value, t);
    }

    static glm::quat sampleQuat(const std::vector<KeyframeQuat>& keys, float time) {
        if (keys.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (keys.size() == 1) return keys[0].value;
        int i = findKeyBefore(time, keys.size(), [&](int k) { return keys[k].time; });
        int j = std::min(i + 1, static_cast<int>(keys.size()) - 1);
        if (i == j) return keys[i].value;
        float t = (time - keys[i].time) / (keys[j].time - keys[i].time);
        return glm::slerp(keys[i].value, keys[j].value, t);
    }

    static SkinningMatrices computeSkinning(const Skeleton& skeleton, const AnimationClip& clip, float time) {
        const size_t boneCount = std::min(skeleton.getBoneCount(), static_cast<size_t>(MAX_BONES));
        const auto& bones = skeleton.getBones();

        // Local pose matrices per bone — default to bind pose so unkeyed bones stay in place.
        std::vector<glm::mat4> localPose(boneCount);
        for (size_t i = 0; i < boneCount; ++i)
            localPose[i] = bones[i].localBindTransform;

        for (const auto& track: clip.getTracks()) {
            if (track.boneIndex < 0 || static_cast<size_t>(track.boneIndex) >= boneCount) continue;
            const glm::mat4& bindLocal = bones[track.boneIndex].localBindTransform;

            // Decompose bind pose as fallback for channels the clip doesn't drive.
            glm::vec3 bindPos(bindLocal[3]);
            glm::vec3 bindScale(glm::length(bindLocal[0]), glm::length(bindLocal[1]), glm::length(bindLocal[2]));
            glm::mat3 rotMat(glm::vec3(bindLocal[0]) / bindScale.x,
                             glm::vec3(bindLocal[1]) / bindScale.y,
                             glm::vec3(bindLocal[2]) / bindScale.z);
            glm::quat bindRot(rotMat);

            glm::vec3 pos = track.positionKeys.empty() ? bindPos : sampleVec3(track.positionKeys, time);
            glm::quat rot = track.rotationKeys.empty() ? bindRot : sampleQuat(track.rotationKeys, time);
            glm::vec3 scale = track.scaleKeys.empty() ? bindScale : sampleVec3(track.scaleKeys, time);

            localPose[track.boneIndex] =
                    glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scale);
        }

        // Accumulate global transforms (bones are ordered parent-before-child by buildSkeleton).
        std::vector<glm::mat4> globalPose(boneCount);
        for (size_t i = 0; i < boneCount; ++i) {
            if (bones[i].parentIndex < 0)
                globalPose[i] = localPose[i];
            else
                globalPose[i] = globalPose[bones[i].parentIndex] * localPose[i];
        }

        SkinningMatrices out{};
        for (size_t i = 0; i < boneCount; ++i)
            out.bones[i] = globalPose[i] * bones[i].offsetMatrix;
        return out;
    }

    AssetLoader& loader_;
    std::unordered_map<EntityHandle, SkinningMatrices> skinningCache_;
    SkinningMatrices identity_ = [] {
        SkinningMatrices m{};
        for (auto& b: m.bones)
            b = glm::mat4(1.0f);
        return m;
    }();
};
