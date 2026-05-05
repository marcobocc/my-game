#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../../../engine/data/components/Camera.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "EditorPickingSystem.hpp"

class EditorState;
class AssetManager;
struct AABB;
struct BoundingSphere;

class GizmoController {
public:
    GizmoController(EditorState& state, AssetManager& assetManager);

    void drawGizmoLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    void drawGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color);
    void drawGizmoAABB(const AABB& aabb, const glm::vec3& color);
    void drawGizmoObjectAABB(const std::string& objectId, const glm::vec3& color);
    void drawGizmoBoundingSphere(const BoundingSphere& sphere, const glm::vec3& color);
    void drawGizmoObjectBoundingSphere(const std::string& objectId, const glm::vec3& color);
    void drawGizmoObjectTransform(const std::string& objectId, float axisLength);
    void drawGizmoBVH(const glm::vec3& color);

    std::vector<GizmoHandle>
    drawTranslationHandles(const std::string& objectId, const Camera& camera, const Transform& cameraTransform);

    std::vector<GizmoHandle>
    drawRotationHandles(const std::string& objectId, const Camera& camera, const Transform& cameraTransform);

    std::vector<GizmoHandle>
    drawScaleHandles(const std::string& objectId, const Camera& camera, const Transform& cameraTransform);

private:
    EditorState& editorState_;
    AssetManager& assetManager_;
};
