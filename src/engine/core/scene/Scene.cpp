#include "Scene.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "../../assets/BuiltinAssetNames.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Renderer.hpp"
#include "core/objects/components/Transform.hpp"

Scene::Scene() : componentStorage_(std::make_unique<ComponentStorage>()) {}

std::pair<std::string, bool> Scene::createEmptyObject(std::string name) {
    if (name.empty()) name = generateName();
    auto [it, created] = gameObjects_.try_emplace(name, name, *componentStorage_);
    return std::make_pair(name, created);
}

void Scene::destroyObject(const std::string& name) {
    componentStorage_->destroyAllComponents(name);
    gameObjects_.erase(name);
}

GameObject& Scene::getObject(const std::string& name) { return gameObjects_.at(name); }

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
    transform.scale = options.scale;

    auto& renderer = gameObj.add<Renderer>();
    renderer.meshName = meshName;
    renderer.materialName = options.materialName;

    return objectName;
}

std::string Scene::createCube(const _createMesh_Options& options) {
    return createMesh(PRIMITIVE_GEOMETRY_CUBE, options);
}

std::string Scene::createRectangle2D(const _createMesh_Options& options) {
    return createMesh(PRIMITIVE_GEOMETRY_RECTANGLE2D, options);
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
