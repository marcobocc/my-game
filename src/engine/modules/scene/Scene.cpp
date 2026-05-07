#include "Scene.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "data/assets/Model.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/assets/BuiltinAssetNames.hpp"

Scene::Scene(AssetManager& assetManager) :
    assetManager_(assetManager),
    componentStorage_(std::make_unique<ComponentStorage>()) {}

std::pair<std::string, bool> Scene::createEmptyObject(std::string name) {
    if (name.empty()) name = generateName();
    auto [it, created] = gameObjects_.try_emplace(name, name, *componentStorage_);
    return std::make_pair(name, created);
}

std::pair<std::string, bool> Scene::createGameObjectFromJson(const nlohmann::json& j, const std::string& name) {
    auto result = createEmptyObject(name);
    auto& obj = getObject(name);
    if (j.contains("transform")) obj.add<Transform>() = Transform::deserialize(j["transform"]);
    if (j.contains("camera")) obj.add<Camera>() = Camera::deserialize(j["camera"]);
    if (j.contains("renderer")) obj.add<Renderer>() = Renderer::deserialize(j["renderer"]);
    if (j.contains("boxCollider")) obj.add<BoxCollider>() = BoxCollider::deserialize(j["boxCollider"]);
    return result;
}

nlohmann::json Scene::serializeGameObject(const GameObject& obj) {
    nlohmann::json entry;
    if (obj.has<Transform>()) entry["transform"] = obj.get<Transform>().serialize();
    if (obj.has<Camera>()) entry["camera"] = obj.get<Camera>().serialize();
    if (obj.has<Renderer>()) entry["renderer"] = obj.get<Renderer>().serialize();
    if (obj.has<BoxCollider>()) entry["boxCollider"] = obj.get<BoxCollider>().serialize();
    return entry;
}

void Scene::destroyObject(const std::string& name) {
    componentStorage_->destroyAllComponents(name);
    gameObjects_.erase(name);
}

GameObject& Scene::getObject(const std::string& name) { return gameObjects_.at(name); }

const std::unordered_map<std::string, GameObject>& Scene::getObjects() const { return gameObjects_; }

void Scene::clear() {
    for (const auto& name: gameObjects_ | std::views::keys)
        componentStorage_->destroyAllComponents(name);
    gameObjects_.clear();
}

nlohmann::json Scene::serialize() const {
    nlohmann::json root;
    nlohmann::json& objects = root["objects"];
    for (const auto& [name, obj]: getObjects()) {
        objects[name] = serializeGameObject(obj);
    }
    return root;
}

void Scene::deserialize(const nlohmann::json& root, Scene& scene) {
    scene.clear();
    for (const auto& [name, entry]: root.at("objects").items()) {
        scene.createGameObjectFromJson(entry, name);
    }
}

std::string Scene::generateName() {
    static size_t counter = 0;
    return "gameObject" + std::to_string(++counter);
}

// ------------------------------------------------------------------------------
// BUILT-IN OBJECTS
// ------------------------------------------------------------------------------

std::string Scene::createMesh(const std::string& meshName, const _createMesh_Options& options) {
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

std::string Scene::createCube(const _createMesh_Options& options) {
    return createMesh(PRIMITIVE_GEOMETRY_CUBE, options);
}

std::string Scene::createPlane(const _createMesh_Options& options) {
    return createMesh(PRIMITIVE_GEOMETRY_PLANE, options);
}

std::string Scene::createModel(const std::string& modelName, const _createModel_Options& options) {
    auto* model = assetManager_.get<Model>(modelName);
    return createMesh(model->getMeshName(),
                      {.position = options.position, .scale = options.scale, .materialName = model->getMaterialName()});
}

std::string Scene::createCamera(const _createCamera_Options& options) {
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
