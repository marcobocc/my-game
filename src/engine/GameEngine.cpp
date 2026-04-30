#include "GameEngine.hpp"
#include "core/math/AABB.hpp"
#include "core/math/BVH.hpp"
#include "data/assets/Mesh.hpp"
#include "data/components/Camera.hpp"
#include "data/settings/RendererSettings.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/core/TimeManager.hpp"
#include "systems/input/InputSystem.hpp"
#include "systems/input/PickingSystem.hpp"
#include "systems/physics/PhysicsSystem.hpp"
#include "systems/rendering/RenderSystem.hpp"
#include "systems/rendering/vulkan/VulkanGraphicsBackend.hpp"
#include "systems/scene/SceneSerializer.hpp"

GameEngine::GameEngine(GameWindow& window,
                       TimeManager& time,
                       UserInterface& userInterface,
                       AssetManager& assetManager,
                       InputSystem& inputSystem,
                       PickingSystem& pickingSystem,
                       PhysicsSystem& physicsSystem,
                       Scene& scene,
                       RenderSystem& renderSystem,
                       RendererSettings& rendererSettings,
                       VulkanGraphicsBackend& graphicsBackend) :
    window_(window),
    time_(time),
    userInterface_(userInterface),
    assetManager_(assetManager),
    inputSystem_(inputSystem),
    pickingSystem_(pickingSystem),
    physicsSystem_(physicsSystem),
    scene_(scene),
    renderSystem_(renderSystem),
    rendererSettings_(rendererSettings),
    graphicsBackend_(graphicsBackend) {}

// --------------------------------------------------------
// Game Loop
// --------------------------------------------------------

void GameEngine::run(const std::function<void(double deltaTime)>& gameLoopFunc) {
    while (!shouldClose()) {
        time_.beginFrame();
        float deltaTime = time_.getGameDeltaTime();

        window_.pollEvents();
        inputSystem_.update();
        physicsSystem_.update();

        gameLoopFunc(deltaTime);
        renderSystem_.update(scene_);
        time_.endFrame();
    }
}

bool GameEngine::shouldClose() const { return window_.shouldClose(); }

void GameEngine::requestClose() const { window_.requestClose(); }

// --------------------------------------------------------
// Input API
// --------------------------------------------------------

std::pair<double, double> GameEngine::getMousePosition() const { return inputSystem_.getMousePosition(); }

bool GameEngine::isKeyDown(int key) const { return inputSystem_.isKeyDown(key); }

bool GameEngine::isKeyPressed(int key) const { return inputSystem_.isKeyPressed(key); }

bool GameEngine::isMouseButtonDown(int button) const { return inputSystem_.isMouseButtonDown(button); }

double GameEngine::getScrollDelta() const { return inputSystem_.getScrollDelta(); }

void GameEngine::requestPick(uint32_t x, uint32_t y) const { pickingSystem_.requestPick(x, y); }

std::optional<std::string> GameEngine::getPickResult() const { return pickingSystem_.getPickResult(); }

// --------------------------------------------------------
// Rendering API
// --------------------------------------------------------

void GameEngine::setActiveCamera(const Camera& camera, const Transform& transform) {
    renderSystem_.setActiveCamera(camera, transform);
}

void GameEngine::enableWorldGrid() { rendererSettings_.enableGrid = true; }
void GameEngine::disableWorldGrid() { rendererSettings_.enableGrid = false; }
void GameEngine::toggleWorldGrid() { rendererSettings_.enableGrid = !rendererSettings_.enableGrid; }

void GameEngine::enableLighting() { rendererSettings_.enableLighting = true; }
void GameEngine::disableLighting() { rendererSettings_.enableLighting = false; }
void GameEngine::toggleLighting() { rendererSettings_.enableLighting = !rendererSettings_.enableLighting; }

void GameEngine::drawObjectOutline(const Renderer& renderer, const Transform& transform, std::string objectId) const {
    graphicsBackend_.outline(renderer, transform, std::move(objectId));
}

void GameEngine::GIZMOS_DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) const {
    graphicsBackend_.submitGizmoLine(from, to, color);
}

void GameEngine::GIZMOS_DrawAABB(const AABB& aabb, const glm::vec3& color) const {
    const glm::vec3 min = aabb.min;
    const glm::vec3 max = aabb.max;

    // Bottom vertices
    glm::vec3 v000 = {min.x, min.y, min.z};
    glm::vec3 v001 = {min.x, min.y, max.z};
    glm::vec3 v010 = {max.x, min.y, min.z};
    glm::vec3 v011 = {max.x, min.y, max.z};

    // Top vertices
    glm::vec3 v100 = {min.x, max.y, min.z};
    glm::vec3 v101 = {min.x, max.y, max.z};
    glm::vec3 v110 = {max.x, max.y, min.z};
    glm::vec3 v111 = {max.x, max.y, max.z};

    // gizmo is jagged and not properly showing the lines
    // Bottom face (y = min.y)
    GIZMOS_DrawLine(v000, v010, color);
    GIZMOS_DrawLine(v010, v011, color);
    GIZMOS_DrawLine(v011, v001, color);
    GIZMOS_DrawLine(v001, v000, color);

    // Top face (y = max.y)
    GIZMOS_DrawLine(v100, v110, color);
    GIZMOS_DrawLine(v110, v111, color);
    GIZMOS_DrawLine(v111, v101, color);
    GIZMOS_DrawLine(v101, v100, color);

    // Vertical edges
    GIZMOS_DrawLine(v000, v100, color);
    GIZMOS_DrawLine(v001, v101, color);
    GIZMOS_DrawLine(v010, v110, color);
    GIZMOS_DrawLine(v011, v111, color);
}

void GameEngine::GIZMOS_DrawObjectAABB(const std::string& objectId, const glm::vec3& color) const {
    GameObject& object = scene_.getObject(objectId);
    if (object.has<Transform>() && object.has<Renderer>()) {
        auto transform = object.get<Transform>();
        auto renderer = object.get<Renderer>();
        auto mesh = assetManager_.get<Mesh>(renderer.meshName);
        auto aabb = mesh->getAABB().applyTransform(transform.getModelMatrix());
        GIZMOS_DrawAABB(aabb, color);
    }
}

void GameEngine::GIZMOS_DrawObjectTransform(const std::string& objectId, float axisLength) const {
    GameObject& object = scene_.getObject(objectId);
    if (object.has<Transform>() && object.has<Renderer>()) {
        auto transform = object.get<Transform>();
        glm::vec3 position = transform.position;
        glm::vec3 right = transform.getRight() * axisLength;
        glm::vec3 up = transform.getUp() * axisLength;
        glm::vec3 forward = transform.getForward() * axisLength;
        GIZMOS_DrawLine(position, position + right, {1.0f, 0.0f, 0.0f});
        GIZMOS_DrawLine(position, position + up, {0.0f, 1.0f, 0.0f});
        GIZMOS_DrawLine(position, position + forward, {0.0f, 0.0f, 1.0f});
    }
}

void GameEngine::GIZMOS_DrawBVH(const glm::vec3& color) const {
    std::vector<Item> items;
    for (const auto& [id, object]: scene_.getObjects()) {
        if (object.has<Transform>() && object.has<Renderer>()) {
            auto transform = object.get<Transform>();
            auto renderer = object.get<Renderer>();
            auto* mesh = assetManager_.get<Mesh>(renderer.meshName);
            if (!mesh) continue;
            auto aabb = mesh->getAABB().applyTransform(transform.getModelMatrix());
            items.push_back({aabb, id});
        }
    }
    if (items.empty()) return;
    BVH bvh;
    bvh.build(std::move(items));
    for (const auto& node: bvh.nodes) {
        GIZMOS_DrawAABB(node.bounds, color);
        if (node.isLeaf()) {
            for (uint32_t i = node.begin; i < node.begin + node.count; i++)
                GIZMOS_DrawAABB(bvh.items[i].aabb, color);
        }
    }
}

// --------------------------------------------------------
// Scene API
// --------------------------------------------------------

std::string GameEngine::createCamera(const Scene::_createCamera_Options& options) {
    return scene_.createCamera(options);
}

std::string GameEngine::createCube(const Scene::_createMesh_Options& options) { return scene_.createCube(options); }

std::string GameEngine::createRectangle2D(const Scene::_createMesh_Options& options) {
    return scene_.createRectangle2D(options);
}

std::string GameEngine::createMesh(const std::string& meshName, const Scene::_createMesh_Options& options) {
    return scene_.createMesh(meshName, options);
}

std::string GameEngine::createModel(const std::string& modelName, const Scene::_createModel_Options& options) {
    return scene_.createModel(modelName, options);
}

void GameEngine::destroyObject(const std::string& name) { scene_.destroyObject(name); }

GameObject& GameEngine::getObject(const std::string& name) const { return scene_.getObject(name); }

const std::unordered_map<std::string, GameObject>& GameEngine::getObjects() const { return scene_.getObjects(); }

nlohmann::json GameEngine::serializeScene() const { return SceneSerializer::serializeScene(scene_); }

void GameEngine::deserializeScene(const nlohmann::json& j) { SceneSerializer::deserializeScene(j, scene_); }

// --------------------------------------------------------
// Assets API
// --------------------------------------------------------

std::vector<std::string> GameEngine::getAvailableAssets(const std::string& extension) const {
    return assetManager_.getAvailableAssets(extension);
}
