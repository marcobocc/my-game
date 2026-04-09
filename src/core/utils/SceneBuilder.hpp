#pragma once
#include <string>
#include "core/GameEngine.hpp"
#include "core/components/CameraComponent.hpp"
#include "core/components/RenderableComponent.hpp"
#include "core/components/TransformComponent.hpp"
#include "core/utils/MaterialBuilder.hpp"
#include "core/utils/PrimitiveBuilder.hpp"

class SceneBuilder {
public:
    explicit SceneBuilder(GameEngine& engine) :
        engine_(engine),
        assetManager_(engine.getAssetManager()),
        primitiveBuilder_(assetManager_),
        materialBuilder_(assetManager_) {}

    unsigned int addCube(const glm::vec3& position = {0.0f, 0.0f, 0.0f},
                         const glm::vec3& scale = {1.0f, 1.0f, 1.0f},
                         const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f}) const {

        std::string meshName = primitiveBuilder_.cube();
        std::string materialName = materialBuilder_.solidColor(color);
        RenderableComponent renderableComponent{meshName, materialName};

        unsigned int entityId = engine_.getECS().createEntity();
        engine_.getECS().addComponent(entityId, std::move(createTransform(position, scale)));
        engine_.getECS().addComponent(entityId, std::move(renderableComponent));
        return entityId;
    }

    unsigned int addTriangle(const glm::vec3& position = {0.0f, 0.0f, 0.0f},
                             const glm::vec3& scale = {1.0f, 1.0f, 1.0f},
                             const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f}) const {

        std::string meshName = primitiveBuilder_.triangle();
        std::string materialName = materialBuilder_.solidColor(color);
        RenderableComponent renderableComponent{meshName, materialName};

        unsigned int entityId = engine_.getECS().createEntity();
        engine_.getECS().addComponent(entityId, std::move(createTransform(position, scale)));
        engine_.getECS().addComponent(entityId, std::move(renderableComponent));
        return entityId;
    }

    unsigned int addMesh(std::unique_ptr<Mesh> mesh,
                         std::unique_ptr<Material> material,
                         const glm::vec3& position = {0.0f, 0.0f, 0.0f},
                         const glm::vec3& scale = {1.0f, 1.0f, 1.0f}) const {

        RenderableComponent renderableComponent{mesh->name, material->name};
        assetManager_.insertMesh(std::move(mesh));
        assetManager_.insertMaterial(std::move(material));

        unsigned int entityId = engine_.getECS().createEntity();
        engine_.getECS().addComponent(entityId, std::move(createTransform(position, scale)));
        engine_.getECS().addComponent(entityId, std::move(renderableComponent));
        return entityId;
    }

    unsigned int addCamera(const glm::vec3& position = {0.0f, 0.0f, 0.0f},
                           const glm::vec3& forward = {0.0f, 0.0f, -1.0f},
                           const glm::vec3& up = {0.0f, 1.0f, 0.0f},
                           float fov = 45.0f,
                           float aspect = 16.0f / 9.0f,
                           float nearPlane = 0.1f,
                           float farPlane = 100.0f) const {

        CameraComponent camera{};
        camera.position = position;
        camera.forward = forward;
        camera.up = up;
        camera.fov = fov;
        camera.aspect = aspect;
        camera.nearPlane = nearPlane;
        camera.farPlane = farPlane;

        unsigned int entityId = engine_.getECS().createEntity();
        engine_.getECS().addComponent(entityId, std::move(camera));
        return entityId;
    }

private:
    GameEngine& engine_;
    AssetManager& assetManager_;
    PrimitiveBuilder primitiveBuilder_;
    MaterialBuilder materialBuilder_;

    static TransformComponent createTransform(const glm::vec3& position, const glm::vec3& scale) {
        TransformComponent transform{};
        transform.position = position;
        transform.scale = scale;
        return transform;
    }
};
