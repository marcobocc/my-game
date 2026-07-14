#include "TerrainPaintTool.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "../../../../../runtime/src/core/components/Transform.hpp"
#include "../../../../../runtime/src/graphics/components/Renderer.hpp"
#include "../../../../../runtime/src/graphics/debug/DebugDraw.hpp"
#include "../../rendering/VulkanBackend.hpp"
#include "core/assets/AssetLoader.hpp"

namespace {
    struct TriangleHit {
        float t;
        float u, v; // barycentric weights of v1 and v2
    };

    // Möller-Trumbore ray-triangle intersection, shared with TerrainSculptTool's approach.
    std::optional<TriangleHit> intersectTriangle(const glm::vec3& origin,
                                                 const glm::vec3& dir,
                                                 const glm::vec3& v0,
                                                 const glm::vec3& v1,
                                                 const glm::vec3& v2) {
        constexpr float EPSILON = 1e-6f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(dir, edge2);
        float a = glm::dot(edge1, h);
        if (std::abs(a) < EPSILON) return std::nullopt;

        float f = 1.0f / a;
        glm::vec3 s = origin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) return std::nullopt;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(dir, q);
        if (v < 0.0f || u + v > 1.0f) return std::nullopt;

        float t = f * glm::dot(edge2, q);
        if (t < EPSILON) return std::nullopt;
        return TriangleHit{t, u, v};
    }
} // namespace

std::optional<TerrainPaintTool::TargetTerrain> TerrainPaintTool::findTarget() const {
    for (EntityHandle entity: selection_.getSelectedEntityIds()) {
        const Actor* actor = world_.getActor(entity);
        if (!actor) continue;
        const Renderer* renderer = actor->getComponent<Renderer>();
        const Transform* transform = actor->getComponent<Transform>();
        if (!renderer || !transform) continue;
        if (!assetStore_.exists(renderer->meshName) || !assetStore_.exists(renderer->materialName)) continue;

        const Material& material = assetStore_.getAsset<Material>(renderer->materialName);
        if (!material.isTerrainBlend()) continue;

        const Mesh& mesh = assetStore_.getAsset<Mesh>(renderer->meshName);
        const auto& positions = mesh.getPositions();
        if (positions.empty()) continue;

        TargetTerrain target{};
        target.entity = entity;
        target.meshName = renderer->meshName;
        target.splatMapTexture = material.splatMapTexture;
        target.modelMatrix = transform->getModelMatrix();
        target.invModelMatrix = glm::inverse(target.modelMatrix);
        target.minX = target.maxX = positions[0].x;
        target.minZ = target.maxZ = positions[0].z;
        for (const auto& p: positions) {
            target.minX = std::min(target.minX, p.x);
            target.maxX = std::max(target.maxX, p.x);
            target.minZ = std::min(target.minZ, p.z);
            target.maxZ = std::max(target.maxZ, p.z);
        }
        return target;
    }
    return std::nullopt;
}

std::optional<TerrainPaintTool::LocalHit> TerrainPaintTool::raycastLocal(const TargetTerrain& target) const {
    const Mesh& mesh = assetStore_.getAsset<Mesh>(target.meshName);

    auto [mx, my] = inputSystem_.getMousePosition();
    auto sv = window_.getSceneViewport();
    float ndcX = (static_cast<float>(mx) - static_cast<float>(sv.x)) / static_cast<float>(sv.width);
    float ndcY = (static_cast<float>(my) - static_cast<float>(sv.y)) / static_cast<float>(sv.height);
    if (ndcX < 0.0f || ndcX > 1.0f || ndcY < 0.0f || ndcY > 1.0f) return std::nullopt;

    float clipX = ndcX * 2.0f - 1.0f;
    float clipY = ndcY * 2.0f - 1.0f;
    glm::mat4 invVP =
            glm::inverse(camera_.getCamera().getProjectionMatrix() * camera_.getCameraTransform().getViewMatrix());
    glm::vec4 nearPoint = invVP * glm::vec4(clipX, clipY, -1.0f, 1.0f);
    glm::vec4 farPoint = invVP * glm::vec4(clipX, clipY, 1.0f, 1.0f);
    glm::vec3 nearWorld = glm::vec3(nearPoint) / nearPoint.w;
    glm::vec3 farWorld = glm::vec3(farPoint) / farPoint.w;

    glm::vec3 originLocal = glm::vec3(target.invModelMatrix * glm::vec4(nearWorld, 1.0f));
    glm::vec3 farLocal = glm::vec3(target.invModelMatrix * glm::vec4(farWorld, 1.0f));
    glm::vec3 dirLocal = glm::normalize(farLocal - originLocal);

    const auto& positions = mesh.getPositions();
    const auto& indices = mesh.getIndices();
    const auto& uvs = mesh.getUvs();
    if (uvs.size() != positions.size()) return std::nullopt;

    std::optional<float> closestT;
    glm::vec2 hitUV(0.0f);
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
        auto hit = intersectTriangle(originLocal, dirLocal, positions[i0], positions[i1], positions[i2]);
        if (hit && (!closestT || hit->t < *closestT)) {
            closestT = hit->t;
            // Interpolate the mesh's actual UVs — the same coordinates the terrain
            // shader samples the splatmap with.
            hitUV = uvs[i0] * (1.0f - hit->u - hit->v) + uvs[i1] * hit->u + uvs[i2] * hit->v;
        }
    }
    if (!closestT) return std::nullopt;
    return LocalHit{originLocal + dirLocal * (*closestT), hitUV};
}

void TerrainPaintTool::applyBrush(Texture& tex, const TargetTerrain& target, const LocalHit& hit, float deltaTime) {
    float width = std::max(target.maxX - target.minX, 1e-4f);
    float depth = std::max(target.maxZ - target.minZ, 1e-4f);
    float speed = strength_ * deltaTime;

    // Convert brush radius (world units) into texel-space radii along each axis.
    // UVs span [0,1] across the mesh's XZ bounds regardless of their orientation.
    float radiusU = radius_ / width * static_cast<float>(tex.width);
    float radiusV = radius_ / depth * static_cast<float>(tex.height);

    float hitU = hit.uv.x * static_cast<float>(tex.width);
    float hitV = hit.uv.y * static_cast<float>(tex.height);

    int minPx = std::max(0, static_cast<int>(std::floor(hitU - radiusU)));
    int maxPx = std::min(tex.width - 1, static_cast<int>(std::ceil(hitU + radiusU)));
    int minPy = std::max(0, static_cast<int>(std::floor(hitV - radiusV)));
    int maxPy = std::min(tex.height - 1, static_cast<int>(std::ceil(hitV + radiusV)));

    for (int py = minPy; py <= maxPy; ++py) {
        for (int px = minPx; px <= maxPx; ++px) {
            float du = (static_cast<float>(px) + 0.5f - hitU) / std::max(radiusU, 1e-4f);
            float dv = (static_cast<float>(py) + 0.5f - hitV) / std::max(radiusV, 1e-4f);
            float dist = std::sqrt(du * du + dv * dv);
            if (dist > 1.0f) continue;
            float falloff = 1.0f - dist;

            size_t idx = (static_cast<size_t>(py) * tex.width + px) * 4;
            std::array<float, 4> weights{};
            float sum = 0.0f;
            for (int c = 0; c < 4; ++c) {
                weights[c] = static_cast<float>(tex.imageData[idx + c]) / 255.0f;
                sum += weights[c];
            }
            if (sum > 1e-6f) {
                for (auto& w: weights)
                    w /= sum;
            }

            float amount = std::clamp(falloff * speed, 0.0f, 1.0f);
            for (int c = 0; c < 4; ++c) {
                float targetWeight = c == activeLayer_ ? 1.0f : 0.0f;
                weights[c] += (targetWeight - weights[c]) * amount;
            }
            sum = weights[0] + weights[1] + weights[2] + weights[3];
            if (sum > 1e-6f) {
                for (int c = 0; c < 4; ++c)
                    tex.imageData[idx + c] =
                            static_cast<unsigned char>(std::clamp(weights[c] / sum * 255.0f, 0.0f, 255.0f));
            }
        }
    }
}

void TerrainPaintTool::beginStroke(const TargetTerrain& target) {
    stroke_ = Stroke{target.splatMapTexture, assetStore_.getAsset<Texture>(target.splatMapTexture)};
}

void TerrainPaintTool::endStroke() {
    if (!stroke_) return;
    std::string splatMapName = stroke_->splatMapName;
    Texture finalTexture = stroke_->workingTexture;
    assetStore_.mutateAsset<Texture>(
            splatMapName,
            [&finalTexture](Texture& t) { t = finalTexture; },
            UndoHistory::randomGroupId("Paint Terrain"));
    stroke_.reset();
}

void TerrainPaintTool::update(DebugDraw& debugDraw) {
    if (!active_) return;

    auto target = findTarget();
    if (!target) {
        if (stroke_) endStroke();
        wasLeftDown_ = false;
        return;
    }

    auto localHit = raycastLocal(*target);
    bool leftDown = inputSystem_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

    if (localHit) {
        constexpr int SEGMENTS = 32;
        glm::vec3 prev;
        for (int i = 0; i <= SEGMENTS; ++i) {
            float angle = static_cast<float>(i) / static_cast<float>(SEGMENTS) * glm::two_pi<float>();
            glm::vec3 local =
                    localHit->position + glm::vec3(std::cos(angle) * radius_, 0.0f, std::sin(angle) * radius_);
            glm::vec3 world = glm::vec3(target->modelMatrix * glm::vec4(local, 1.0f));
            if (i > 0) debugDraw.line(prev, world, glm::vec3(0.9f, 0.3f, 0.9f));
            prev = world;
        }
    }

    if (leftDown && localHit) {
        if (!stroke_) beginStroke(*target);
        if (stroke_ && stroke_->splatMapName == target->splatMapTexture) {
            applyBrush(stroke_->workingTexture, *target, *localHit, 1.0f / 60.0f);
            backend_.getResourcesManager().updateTexture(stroke_->workingTexture);
        }
    } else if (wasLeftDown_ && !leftDown) {
        endStroke();
    }

    wasLeftDown_ = leftDown;
}
