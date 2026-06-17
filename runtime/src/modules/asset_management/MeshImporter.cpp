#include "MeshImporter.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <stdexcept>
#include "VirtualFileSystem.hpp"

namespace importing {

    std::vector<std::unique_ptr<Mesh>> importMeshFile(const std::filesystem::path& filepath,
                                                      bool reverseWinding,
                                                      const std::string& namePrefix,
                                                      const VirtualFileSystem& vfs) {
        auto realPathOpt = vfs.getRealPath(filepath.string());
        if (!realPathOpt) {
            throw std::runtime_error("Mesh file not found in VFS: " + filepath.string());
        }

        Assimp::Importer importer;

        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
                             aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_SortByPType;

        if (reverseWinding) flags |= aiProcess_FlipWindingOrder;

        const aiScene* scene = importer.ReadFile(realPathOpt->string(), flags);
        if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
            throw std::runtime_error("Assimp failed to load: " + filepath.string() + " - " + importer.GetErrorString());
        }

        std::vector<std::unique_ptr<Mesh>> meshes;
        meshes.reserve(scene->mNumMeshes);

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
            const aiMesh* aim = scene->mMeshes[m];

            std::vector<glm::vec3> positions;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec4> tangents;
            std::vector<glm::vec3> colors;
            std::vector<uint32_t> indices;

            positions.reserve(aim->mNumVertices);
            for (unsigned int i = 0; i < aim->mNumVertices; ++i) {
                positions.emplace_back(aim->mVertices[i].x, aim->mVertices[i].y, aim->mVertices[i].z);
            }

            if (aim->HasTextureCoords(0)) {
                uvs.reserve(aim->mNumVertices);
                for (unsigned int i = 0; i < aim->mNumVertices; ++i) {
                    uvs.emplace_back(aim->mTextureCoords[0][i].x, aim->mTextureCoords[0][i].y);
                }
            }

            if (aim->HasNormals()) {
                normals.reserve(aim->mNumVertices);
                for (unsigned int i = 0; i < aim->mNumVertices; ++i) {
                    normals.emplace_back(aim->mNormals[i].x, aim->mNormals[i].y, aim->mNormals[i].z);
                }
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
                for (unsigned int i = 0; i < aim->mNumVertices; ++i) {
                    colors.emplace_back(aim->mColors[0][i].r, aim->mColors[0][i].g, aim->mColors[0][i].b);
                }
            }

            indices.reserve(aim->mNumFaces * 3);
            for (unsigned int i = 0; i < aim->mNumFaces; ++i) {
                for (unsigned int j = 0; j < aim->mFaces[i].mNumIndices; ++j) {
                    indices.push_back(aim->mFaces[i].mIndices[j]);
                }
            }

            std::string meshName = scene->mNumMeshes > 1 ? namePrefix + "_" + std::to_string(m) : namePrefix;

            meshes.push_back(std::make_unique<Mesh>(meshName,
                                                    std::move(positions),
                                                    std::move(uvs),
                                                    std::move(colors),
                                                    std::move(indices),
                                                    std::move(normals),
                                                    std::move(tangents)));
        }

        return meshes;
    }

} // namespace importing
