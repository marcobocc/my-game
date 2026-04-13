#include "Scene.hpp"
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

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

std::string Scene::createCube(const glm::vec3& position, const glm::vec3& scale) {
    auto [objectName, successful] = createEmptyObject();
    auto& gameObj = getObject(objectName);

    auto& transform = gameObj.add<Transform>();
    transform.position = position;
    transform.scale = scale;

    auto& [name] = gameObj.add<Mesh>();
    name = PRIMITIVE_GEOMETRY_CUBE;

    auto& [shaderName, baseColor] = gameObj.add<Material>();
    baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    return objectName;
}

std::string Scene::createCamera(const glm::vec3& position,
                                const glm::vec3& forward,
                                const glm::vec3& up,
                                float fov,
                                float aspect,
                                float nearPlane,
                                float farPlane) {
    auto [objectName, created] = createEmptyObject();
    auto& obj = getObject(objectName);
    auto& camera = obj.add<Camera>();
    camera = Camera{.position = position,
                    .forward = forward,
                    .up = up,
                    .fov = fov,
                    .aspect = aspect,
                    .nearPlane = nearPlane,
                    .farPlane = farPlane};
    return objectName;
}
