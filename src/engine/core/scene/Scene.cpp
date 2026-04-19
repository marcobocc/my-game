#include "Scene.hpp"
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "assets/types/mesh/details/PrefabNames.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Material.hpp"
#include "core/objects/components/Mesh.hpp"

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

std::string Scene::createCube(const _createCube_Options& options) {
    auto [objectName, successful] = createEmptyObject();
    auto& gameObj = getObject(objectName);

    auto& transform = gameObj.add<Transform>();
    transform.position = options.position;
    transform.scale = options.scale;

    auto& [name] = gameObj.add<Mesh>();
    name = PRIMITIVE_GEOMETRY_CUBE;

    auto& [shaderName, baseColor] = gameObj.add<Material>();
    baseColor = options.color;
    return objectName;
}

std::string Scene::createMeshObject(const std::string& meshName, const _createMeshObject_Options& options) {
    auto [objectName, successful] = createEmptyObject();
    auto& gameObj = getObject(objectName);

    auto& transform = gameObj.add<Transform>();
    transform.position = options.position;
    transform.scale = options.scale;

    auto& [name] = gameObj.add<Mesh>();
    name = meshName;

    auto& [shaderName, baseColor] = gameObj.add<Material>();
    baseColor = options.color;
    return objectName;
}

std::string Scene::createCamera(const _createCamera_Options& options) {
    auto [objectName, created] = createEmptyObject();
    auto& obj = getObject(objectName);
    auto& camera = obj.add<Camera>();
    camera = Camera{.position = options.position,
                    .forward = options.forward,
                    .up = options.up,
                    .fov = options.fov,
                    .aspect = options.aspect,
                    .nearPlane = options.nearPlane,
                    .farPlane = options.farPlane};
    return objectName;
}
