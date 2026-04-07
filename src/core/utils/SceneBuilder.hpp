#pragma once
#include <string>
#include "GameEngine.hpp"
#include "MaterialBuilder.hpp"
#include "PrimitiveBuilder.hpp"
#include "core/components/CameraComponent.hpp"
#include "core/components/RenderableComponent.hpp"
#include "core/components/TransformComponent.hpp"

class SceneBuilder {
public:
    explicit SceneBuilder(GameEngine& engine) :
        engine_(engine),
        primitiveBuilder_(engine.getAssetManager()),
        materialBuilder_(engine.getAssetManager()) {}

    unsigned int createCube(const glm::vec3& position = {0.0f, 0.0f, 0.0f},
                            const glm::vec3& scale = {1.0f, 1.0f, 1.0f},
                            const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f}) const {

        std::string meshName = primitiveBuilder_.createCube();
        std::string materialName = materialBuilder_.createMaterialColor(color);
        RenderableComponent renderableComponent{meshName, materialName};

        unsigned int entityId = engine_.getECS().createEntity();
        engine_.getECS().addComponent(entityId, std::move(createTransform(position, scale)));
        engine_.getECS().addComponent(entityId, std::move(renderableComponent));
        return entityId;
    }

    unsigned int createTriangle(const glm::vec3& position = {0.0f, 0.0f, 0.0f},
                                const glm::vec3& scale = {1.0f, 1.0f, 1.0f},
                                const glm::vec4& color = {1.0f, 1.0f, 1.0f, 1.0f}) const {

        std::string meshName = primitiveBuilder_.createTriangle();
        std::string materialName = materialBuilder_.createMaterialColor(color);
        RenderableComponent renderableComponent{meshName, materialName};

        unsigned int entityId = engine_.getECS().createEntity();
        engine_.getECS().addComponent(entityId, std::move(createTransform(position, scale)));
        engine_.getECS().addComponent(entityId, std::move(renderableComponent));
        return entityId;
    }

    unsigned int createCamera(const glm::vec3& position = {0.0f, 0.0f, 0.0f},
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
    PrimitiveBuilder primitiveBuilder_;
    MaterialBuilder materialBuilder_;

    static TransformComponent createTransform(const glm::vec3& position, const glm::vec3& scale) {
        TransformComponent transform{};
        transform.position = position;
        transform.scale = scale;
        return transform;
    }
};
