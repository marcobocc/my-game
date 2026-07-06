#pragma once
#include <log4cxx/logger.h>
#include "GameRenderData.hpp"
#include "IGameRenderer.hpp"
#include "modules/animation/AnimationSystem.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/scene/TransformUtils.hpp"
#include "modules/scene/World.hpp"
#include "modules/scene/components/Animator.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/ParticleEmitter.hpp"
#include "modules/scene/components/Renderer.hpp"
#include "modules/scene/components/TextComponent.hpp"
#include "modules/scene/components/Transform.hpp"

class GameRenderSystem {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("GameRenderSystem");

public:
    explicit GameRenderSystem(IGameRenderer& renderer) : renderer_(renderer) {}

    void setAnimationSystem(AnimationSystem* animationSystem) { animationSystem_ = animationSystem; }

    void setActiveCamera(const Camera& camera, const Transform& cameraTransform) {
        activeCamera_ = &camera;
        activeCameraTransform_ = &cameraTransform;
    }

    void setDeltaTime(float dt) {
        deltaTime_ = dt;
        renderer_.setDeltaTime(dt);
    }

    void update(const World& entityManager) {
        std::vector<DrawCall> drawQueue;
        auto drawables = entityManager.query<Renderer, Transform>();
        for (auto& [entity, rendererPtr, transformPtr]: drawables) {
            if (!rendererPtr || !transformPtr) continue;
            if (!rendererPtr->enabled) continue;
            const glm::mat4* skinBones = nullptr;
            if (animationSystem_ && animationSystem_->hasSkinning(entity))
                skinBones = animationSystem_->getSkinningMatrices(entity).bones;
            drawQueue.push_back({const_cast<Renderer&>(*rendererPtr),
                                 const_cast<Transform&>(*transformPtr),
                                 TransformUtils::resolveWorldMatrix(*transformPtr, entityManager),
                                 std::to_string(entity),
                                 skinBones});
        }

        std::vector<std::pair<Light, Transform>> lightsWithTransforms;
        auto lights = entityManager.query<Light, Transform>();
        for (auto& [entity, lightPtr, transformPtr]: lights) {
            if (!lightPtr || !transformPtr) continue;
            lightsWithTransforms.emplace_back(*lightPtr, *transformPtr);
        }

        std::vector<ParticleEmitterRef> particleEmitters;
        auto emitters = entityManager.query<ParticleEmitter, Transform>();
        for (auto& [entity, emitterPtr, transformPtr]: emitters) {
            if (!emitterPtr || !transformPtr) continue;
            auto* e = const_cast<ParticleEmitter*>(emitterPtr);
            e->spawnAccumulator += deltaTime_ * e->emissionRate;
            auto toSpawn = static_cast<uint32_t>(e->spawnAccumulator);
            e->spawnAccumulator -= static_cast<float>(toSpawn);
            toSpawn = std::min(toSpawn, e->maxParticles);
            particleEmitters.push_back({e, transformPtr->position, toSpawn});
        }

        std::vector<TextDrawCall> textQueue;
        auto texts = entityManager.query<TextComponent, Transform>();
        for (auto& [entity, textComp, transformPtr]: texts) {
            if (!textComp->visible || textComp->text.empty()) continue;
            textQueue.push_back({textComp->text,
                                 textComp->fontName.empty() ? std::string(DEFAULT_FONT) : textComp->fontName,
                                 glm::vec3(TransformUtils::resolveWorldMatrix(*transformPtr, entityManager)[3]),
                                 textComp->color,
                                 textComp->fontSize,
                                 textComp->billboard,
                                 textComp->alignment});
        }

        const Camera& cam = getActiveCamera();
        const Transform& camTransform = getActiveCameraTransform();
        GameRenderData rd{
                cam, camTransform, drawQueue, lightsWithTransforms, std::move(particleEmitters), std::move(textQueue)};
        renderer_.renderFrame(rd);
    }

private:
    Camera getActiveCamera() const {
        if (activeCamera_ != nullptr) return *activeCamera_;
        LOG4CXX_WARN(LOGGER, "Active camera not set for GameRenderSystem. Using default camera.");
        return Camera();
    }

    Transform getActiveCameraTransform() const {
        if (activeCameraTransform_ != nullptr) return *activeCameraTransform_;
        LOG4CXX_WARN(LOGGER, "Active camera transform not set for GameRenderSystem. Using default camera transform.");
        return Transform();
    }

    IGameRenderer& renderer_;
    AnimationSystem* animationSystem_ = nullptr;
    const Camera* activeCamera_ = nullptr;
    const Transform* activeCameraTransform_ = nullptr;
    float deltaTime_ = 0.0f;
};
