#include "SceneManager.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "data/assets/Model.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/assets/BuiltinAssetNames.hpp"

SceneManager::SceneManager(AssetManager& assetManager) :
    assetManager_(assetManager),
    componentStorage_(std::make_unique<ComponentStorage>()) {}

std::pair<std::string, bool> SceneManager::createEmptyObject(std::string name) {
    if (name.empty()) name = generateName();
    auto [it, created] = gameObjects_.try_emplace(name, name, *componentStorage_);
    return std::make_pair(name, created);
}

void SceneManager::destroyObject(const std::string& name) {
    componentStorage_->destroyAllComponents(name);
    gameObjects_.erase(name);
}

void SceneManager::clear() {
    for (auto& [name, _]: gameObjects_)
        componentStorage_->destroyAllComponents(name);
    gameObjects_.clear();
}

GameObject& SceneManager::getObject(const std::string& name) { return gameObjects_.at(name); }

const std::unordered_map<std::string, GameObject>& SceneManager::getObjects() const { return gameObjects_; }

std::string SceneManager::generateName() {
    static size_t counter = 0;
    return "gameObject" + std::to_string(++counter);
}

// ------------------------------------------------------------------------------
// BUILT-IN OBJECTS
// ------------------------------------------------------------------------------

std::string SceneManager::createMesh(const std::string& meshName, const _createMesh_Options& options) {
    auto [objectName, successful] = createEmptyObject();
    auto& gameObj = getObject(objectName);

    auto& transform = gameObj.add<Transform>();
    transform.position = options.position;
    transform.rotation = options.rotation;
    transform.scale = options.scale;

    auto& renderer = gameObj.add<Renderer>();
    renderer.meshName = meshName;
    renderer.materialName = options.materialName;

    return objectName;
}

std::string SceneManager::createCube(const _createMesh_Options& options) {
    return createMesh(PRIMITIVE_GEOMETRY_CUBE, options);
}

std::string SceneManager::createRectangle2D(const _createMesh_Options& options) {
    return createMesh(PRIMITIVE_GEOMETRY_PLANE, options);
}

std::string SceneManager::createModel(const std::string& modelName, const _createModel_Options& options) {
    auto* model = assetManager_.get<Model>(modelName);
    return createMesh(model->getMeshName(),
                      {.position = options.position, .scale = options.scale, .materialName = model->getMaterialName()});
}

std::string SceneManager::createCamera(const _createCamera_Options& options) {
    auto [objectName, created] = createEmptyObject();
    auto& obj = getObject(objectName);

    auto& transform = obj.add<Transform>();
    transform.position = options.position;
    transform.rotation = options.rotation;

    auto& camera = obj.add<Camera>();
    camera = Camera{
            .fov = options.fov, .aspect = options.aspect, .nearPlane = options.nearPlane, .farPlane = options.farPlane};
    return objectName;
}
