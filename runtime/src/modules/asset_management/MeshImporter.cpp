#include "MeshImporter.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include "VirtualFileSystem.hpp"

namespace importing {

    namespace {

        glm::mat4 toGlm(const aiMatrix4x4& m) {
            return glm::mat4(
                    m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2, m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4);
        }

        glm::vec3 toGlm(const aiVector3D& v) { return {v.x, v.y, v.z}; }

        glm::quat toGlm(const aiQuaternion& q) { return {q.w, q.x, q.y, q.z}; }

        // Collect all node names that have at least one animation channel across all animations.
        std::unordered_set<std::string> collectAnimatedNodes(const aiScene* scene) {
            std::unordered_set<std::string> animated;
            for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
                const aiAnimation* anim = scene->mAnimations[a];
                for (unsigned int c = 0; c < anim->mNumChannels; ++c)
                    animated.insert(anim->mChannels[c]->mNodeName.C_Str());
            }
            return animated;
        }

        std::unique_ptr<Skeleton> buildSkeleton(const aiScene* scene,
                                                const std::string& skeletonName,
                                                std::unordered_map<std::string, int>& outBoneNameToIndex) {
            // Collect offset matrices for mesh-influencing bones.
            std::unordered_map<std::string, glm::mat4> offsetMatrices;
            for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
                const aiMesh* aim = scene->mMeshes[m];
                for (unsigned int b = 0; b < aim->mNumBones; ++b) {
                    const aiBone* ab = aim->mBones[b];
                    offsetMatrices[ab->mName.C_Str()] = toGlm(ab->mOffsetMatrix);
                }
            }
            if (offsetMatrices.empty()) return nullptr;

            // Include all animated nodes in the skeleton, not just weighted bones.
            // Assimp splits FBX pre/post rotations into helper nodes (_$AssimpFbx$_Rotation etc.)
            // that carry animation channels but have no offsetMatrix — they must be in the hierarchy.
            const std::unordered_set<std::string> animatedNodes = collectAnimatedNodes(scene);

            // Walk the node tree in DFS order (guarantees parent-before-child indexing).
            // localBindTransform is derived from offsetMatrix so it exactly matches what Assimp
            // used internally: localBind[i] = inverse(parentGlobalBind) * inverse(offsetMatrix[i]).
            // For helper nodes (no offsetMatrix) we use the raw node transform.
            std::vector<Bone> bones;

            // For each influencer bone, compute the full global bind pose by walking the raw node tree.
            // This correctly accounts for ALL intermediate nodes (PreRotation, PostRotation, etc.)
            // that Assimp uses when computing offsetMatrix, regardless of whether they're animated.
            std::unordered_map<std::string, glm::mat4> nodeGlobalTransform;
            std::function<void(const aiNode*, glm::mat4)> computeGlobals = [&](const aiNode* node,
                                                                               glm::mat4 parentGlobal) {
                glm::mat4 global = parentGlobal * toGlm(node->mTransformation);
                nodeGlobalTransform[node->mName.C_Str()] = global;
                for (unsigned int i = 0; i < node->mNumChildren; ++i)
                    computeGlobals(node->mChildren[i], global);
            };
            computeGlobals(scene->mRootNode, glm::mat4(1.0f));

            std::function<void(const aiNode*, int)> walk = [&](const aiNode* node, int parentIndex) {
                std::string nodeName = node->mName.C_Str();
                bool isInfluencer = offsetMatrices.count(nodeName) > 0;
                bool isAnimated = animatedNodes.count(nodeName) > 0;
                // Assimp helper nodes (PreRotation, PostRotation, etc.) are baked into offsetMatrix
                // but may not have animation channels; they must still be in the skeleton chain.
                bool isHelper = nodeName.find("$AssimpFbx$") != std::string::npos;

                if (isInfluencer || isAnimated || isHelper) {
                    int myIndex = static_cast<int>(bones.size());
                    outBoneNameToIndex[nodeName] = myIndex;
                    Bone bone;
                    bone.name = nodeName;
                    bone.parentIndex = parentIndex;
                    bone.offsetMatrix = isInfluencer ? offsetMatrices[nodeName] : glm::mat4(1.0f);

                    {
                        // localBindTransform = inverse(parentSkeletonNodeGlobal) * myNodeGlobal.
                        // We use the direct skeleton parent (parentIndex), not the nearest influencer,
                        // because the runtime accumulates through every node in the skeleton chain.
                        glm::mat4 myGlobal = nodeGlobalTransform[nodeName];
                        glm::mat4 parentGlobal =
                                (parentIndex >= 0) ? nodeGlobalTransform[bones[parentIndex].name] : glm::mat4(1.0f);
                        bone.localBindTransform = glm::inverse(parentGlobal) * myGlobal;
                    }

                    bones.push_back(std::move(bone));
                }

                int nextParent =
                        (isInfluencer || isAnimated || isHelper) ? static_cast<int>(bones.size()) - 1 : parentIndex;
                for (unsigned int i = 0; i < node->mNumChildren; ++i)
                    walk(node->mChildren[i], nextParent);
            };
            walk(scene->mRootNode, -1);

            if (bones.empty()) return nullptr;
            return std::make_unique<Skeleton>(skeletonName, std::move(bones));
        }

    } // anonymous namespace

    ImportedMeshFile importMeshFile(const std::filesystem::path& filepath,
                                    bool reverseWinding,
                                    const std::string& namePrefix,
                                    const VirtualFileSystem& vfs) {
        auto realPathOpt = vfs.getRealPath(filepath.string());
        if (!realPathOpt) throw std::runtime_error("Mesh file not found in VFS: " + filepath.string());

        Assimp::Importer importer;

        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
                             aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_SortByPType |
                             aiProcess_LimitBoneWeights | aiProcess_FixInfacingNormals;
        if (reverseWinding) flags |= aiProcess_FlipWindingOrder;

        const aiScene* scene = importer.ReadFile(realPathOpt->string(), flags);
        if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
            throw std::runtime_error("Assimp failed to load: " + filepath.string() + " - " + importer.GetErrorString());

        ImportedMeshFile result;

        // -----------------------------------------------------------------------
        // Skeleton
        // -----------------------------------------------------------------------
        std::unordered_map<std::string, int> boneNameToIndex;
        std::string skeletonName = namePrefix + ".skel";
        result.skeleton = buildSkeleton(scene, skeletonName, boneNameToIndex);
        const bool hasBones = (result.skeleton != nullptr);

        // -----------------------------------------------------------------------
        // Meshes
        // -----------------------------------------------------------------------
        result.meshes.reserve(scene->mNumMeshes);

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
            const aiMesh* aim = scene->mMeshes[m];

            std::vector<glm::vec3> positions;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec4> tangents;
            std::vector<glm::vec3> colors;
            std::vector<uint32_t> indices;

            positions.reserve(aim->mNumVertices);
            for (unsigned int i = 0; i < aim->mNumVertices; ++i)
                positions.emplace_back(aim->mVertices[i].x, aim->mVertices[i].y, aim->mVertices[i].z);

            if (aim->HasTextureCoords(0)) {
                uvs.reserve(aim->mNumVertices);
                for (unsigned int i = 0; i < aim->mNumVertices; ++i)
                    uvs.emplace_back(aim->mTextureCoords[0][i].x, aim->mTextureCoords[0][i].y);
            }

            if (aim->HasNormals()) {
                normals.reserve(aim->mNumVertices);
                for (unsigned int i = 0; i < aim->mNumVertices; ++i)
                    normals.emplace_back(aim->mNormals[i].x, aim->mNormals[i].y, aim->mNormals[i].z);
            }

            if (aim->HasTangentsAndBitangents()) {
                tangents.reserve(aim->mNumVertices);
                for (unsigned int i = 0; i < aim->mNumVertices; ++i) {
                    glm::vec3 t(aim->mTangents[i].x, aim->mTangents[i].y, aim->mTangents[i].z);
                    glm::vec3 b(aim->mBitangents[i].x, aim->mBitangents[i].y, aim->mBitangents[i].z);
                    glm::vec3 n(aim->mNormals[i].x, aim->mNormals[i].y, aim->mNormals[i].z);
                    float w = (glm::dot(glm::cross(n, t), b) < 0.0f) ? -1.0f : 1.0f;
                    tangents.emplace_back(t.x, t.y, t.z, w);
                }
            }

            if (aim->HasVertexColors(0)) {
                colors.reserve(aim->mNumVertices);
                for (unsigned int i = 0; i < aim->mNumVertices; ++i)
                    colors.emplace_back(aim->mColors[0][i].r, aim->mColors[0][i].g, aim->mColors[0][i].b);
            }

            indices.reserve(aim->mNumFaces * 3);
            for (unsigned int i = 0; i < aim->mNumFaces; ++i)
                for (unsigned int j = 0; j < aim->mFaces[i].mNumIndices; ++j)
                    indices.push_back(aim->mFaces[i].mIndices[j]);

            std::string meshName = scene->mNumMeshes > 1 ? namePrefix + "_" + std::to_string(m) : namePrefix;

            auto mesh = std::make_unique<Mesh>(meshName,
                                               std::move(positions),
                                               std::move(uvs),
                                               std::move(colors),
                                               std::move(indices),
                                               std::move(normals),
                                               std::move(tangents));

            // ---- Per-vertex bone influences ----
            if (hasBones && aim->mNumBones > 0) {
                const uint32_t vertCount = aim->mNumVertices;
                std::vector<glm::ivec4> boneIdx(vertCount, glm::ivec4(0));
                std::vector<glm::vec4> boneWt(vertCount, glm::vec4(0.0f));
                std::vector<int> boneSlot(vertCount, 0);

                for (unsigned int b = 0; b < aim->mNumBones; ++b) {
                    const aiBone* ab = aim->mBones[b];
                    auto nameIt = boneNameToIndex.find(ab->mName.C_Str());
                    if (nameIt == boneNameToIndex.end()) continue;
                    int globalBoneIdx = nameIt->second;

                    for (unsigned int w = 0; w < ab->mNumWeights; ++w) {
                        uint32_t vi = ab->mWeights[w].mVertexId;
                        int slot = boneSlot[vi];
                        if (slot >= static_cast<int>(Mesh::MAX_BONE_INFLUENCES)) continue;
                        boneIdx[vi][slot] = globalBoneIdx;
                        boneWt[vi][slot] = ab->mWeights[w].mWeight;
                        ++boneSlot[vi];
                    }
                }

                // Normalize weights so they sum to 1.
                for (uint32_t vi = 0; vi < vertCount; ++vi) {
                    float sum = boneWt[vi].x + boneWt[vi].y + boneWt[vi].z + boneWt[vi].w;
                    if (sum > 1e-6f) boneWt[vi] /= sum;
                }

                mesh->setSkinningData(std::move(boneIdx), std::move(boneWt), skeletonName);
            }

            result.meshes.push_back(std::move(mesh));
        }

        // -----------------------------------------------------------------------
        // Animation clips
        // -----------------------------------------------------------------------
        if (hasBones) {
            for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
                const aiAnimation* anim = scene->mAnimations[a];
                float ticksPerSec = anim->mTicksPerSecond > 0.0 ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;
                float duration = static_cast<float>(anim->mDuration) / ticksPerSec;

                std::vector<BoneTrack> tracks;
                tracks.reserve(anim->mNumChannels);

                for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
                    const aiNodeAnim* ch = anim->mChannels[c];
                    auto nameIt = boneNameToIndex.find(ch->mNodeName.C_Str());
                    if (nameIt == boneNameToIndex.end()) continue;

                    BoneTrack track;
                    track.boneIndex = nameIt->second;

                    track.positionKeys.reserve(ch->mNumPositionKeys);
                    for (unsigned int k = 0; k < ch->mNumPositionKeys; ++k)
                        track.positionKeys.push_back({static_cast<float>(ch->mPositionKeys[k].mTime) / ticksPerSec,
                                                      toGlm(ch->mPositionKeys[k].mValue)});

                    track.rotationKeys.reserve(ch->mNumRotationKeys);
                    for (unsigned int k = 0; k < ch->mNumRotationKeys; ++k)
                        track.rotationKeys.push_back({static_cast<float>(ch->mRotationKeys[k].mTime) / ticksPerSec,
                                                      toGlm(ch->mRotationKeys[k].mValue)});

                    track.scaleKeys.reserve(ch->mNumScalingKeys);
                    for (unsigned int k = 0; k < ch->mNumScalingKeys; ++k)
                        track.scaleKeys.push_back({static_cast<float>(ch->mScalingKeys[k].mTime) / ticksPerSec,
                                                   toGlm(ch->mScalingKeys[k].mValue)});

                    tracks.push_back(std::move(track));
                }

                std::string clipName = namePrefix + "/" + anim->mName.C_Str() + ".anim";
                result.clips.push_back(std::make_unique<AnimationClip>(clipName, duration, std::move(tracks)));
            }
        }

        return result;
    }

} // namespace importing
