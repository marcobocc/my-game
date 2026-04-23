#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "assets/BuiltinAssetNames.hpp"
#include "core/objects/ComponentStorage.hpp"
#include "core/objects/GameObject.hpp"
#include "core/objects/components/Transform.hpp"

class Scene {
public:
    Scene();

    std::pair<std::string, bool> createEmptyObject(std::string name = "");
    void destroyObject(const std::string& name);
    GameObject& getObject(const std::string& name);

    template<typename... Components>
    auto getObjectsWith() const {
        return componentStorage_->query<Components...>();
    }

    // ------------------------------------------------------------------------------
    // createCube
    // ------------------------------------------------------------------------------
    struct _createCube_Options {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        std::string materialName{SOLID_COLOR_MATERIAL};
    };
    std::string createCube(const _createCube_Options& options);

    // ------------------------------------------------------------------------------
    // createMeshObject
    // ------------------------------------------------------------------------------
    struct _createMeshObject_Options {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        std::string materialName{SOLID_COLOR_MATERIAL};
    };
    std::string createMeshObject(const std::string& meshName, const _createMeshObject_Options& options);

    // ------------------------------------------------------------------------------
    // createCamera
    // ------------------------------------------------------------------------------
    struct _createCamera_Options {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::vec3 forward{0.0f, 0.0f, -1.0f};
        glm::vec3 up{0.0f, 1.0f, 0.0f};
        float fov{45.0f};
        float aspect{16.0f / 9.0f};
        float nearPlane{0.1f};
        float farPlane{100.0f};
    };
    std::string createCamera(const _createCamera_Options& options);

private:
    std::unique_ptr<ComponentStorage> componentStorage_;
    std::unordered_map<std::string, GameObject> gameObjects_{};
    static std::string generateName();
};
