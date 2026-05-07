#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>

#include "modules/assets/BuiltinAssetNames.hpp"
#include "modules/scene/ComponentStorage.hpp"
#include "modules/scene/DefaultOptions.hpp"
#include "modules/scene/GameObject.hpp"

class AssetManager;

class Scene {
public:
    explicit Scene(AssetManager& assetManager);

    std::pair<std::string, bool> createEmptyObject(std::string name = "");
    std::pair<std::string, bool> createGameObjectFromJson(const nlohmann::json& j, const std::string& name = "");
    static nlohmann::json serializeGameObject(const GameObject& obj);
    void destroyObject(const std::string& name);

    GameObject& getObject(const std::string& name);
    const std::unordered_map<std::string, GameObject>& getObjects() const;
    template<typename... Components>
    auto getObjectsWith() const {
        return componentStorage_->query<Components...>();
    }

    void clear();
    nlohmann::json serialize() const;
    static void deserialize(const nlohmann::json& root, Scene& scene);

    // ------------------------------------------------------------------------------
    // createMesh
    // ------------------------------------------------------------------------------
    struct _createMesh_Options {
        glm::vec3 position{DEFAULT_POSITION};
        glm::quat rotation{DEFAULT_ROTATION};
        glm::vec3 scale{DEFAULT_SCALE};
        std::string materialName{SOLID_COLOR_MATERIAL};
    };
    std::string createMesh(const std::string& meshName, const _createMesh_Options& options);
    std::string createCube(const _createMesh_Options& options);
    std::string createPlane(const _createMesh_Options& options);

    // ------------------------------------------------------------------------------
    // createModel
    // ------------------------------------------------------------------------------
    struct _createModel_Options {
        glm::vec3 position{DEFAULT_POSITION};
        glm::vec3 scale{DEFAULT_SCALE};
    };
    std::string createModel(const std::string& modelName, const _createModel_Options& options);

    // ------------------------------------------------------------------------------
    // createCamera
    // ------------------------------------------------------------------------------
    struct _createCamera_Options {
        glm::vec3 position{DEFAULT_POSITION};
        glm::quat rotation{DEFAULT_ROTATION};
        float fov{DEFAULT_FOV};
        float aspect{DEFAULT_ASPECT};
        float nearPlane{DEFAULT_NEAR_PLANE};
        float farPlane{DEFAULT_FAR_PLANE};
    };
    std::string createCamera(const _createCamera_Options& options);

private:
    AssetManager& assetManager_;
    std::unique_ptr<ComponentStorage> componentStorage_;
    std::unordered_map<std::string, GameObject> gameObjects_{};
    static std::string generateName();
};
