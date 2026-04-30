#pragma once
#include <filesystem>
#include "GameEngine.hpp"
#include "GameWindow.hpp"
#include "Time.hpp"
#include "assets/AssetManager.hpp"
#include "assets/importing/AssetImporter.hpp"
#include "input/InputSystem.hpp"
#include "physics/PhysicsSystem.hpp"
#include "picking/PickingSystem.hpp"
#include "rendering/RenderSystem.hpp"
#include "rendering/vulkan/VulkanWiringContainer.hpp"
#include "scene/Scene.hpp"
#include "ui/UserInterface.hpp"

/*
    GameEngineWiringContainer

    Purpose:
    --------------------------------------------------
    Central composition root for the game engine.

    Responsibilities:
    --------------------------------------------------
    - Create and wire dependencies for all engine services
    - Expose references to all engine services

    This class is not responsible for:
    --------------------------------------------------
    - Application logic
    - Game loop orchestration
    - Public API for the application layer

    Notes:
    --------------------------------------------------
    This class exists purely to build and hold the object graph.
    It should not contain runtime or domain logic, nor should it
    forward logic to other classes. If application behavior appears
    here, the design is leaking responsibilities.
*/
class GameEngineWiringContainer {
public:
    GameEngineWiringContainer(GameWindow& window, const std::filesystem::path& assetsPath) :
        window_(window),
        rendererSettings_(),
        userInterface_(),
        assetStorage_(),
        assetImporter_(assetsPath, assetStorage_),
        assetManager_(assetImporter_, assetStorage_),
        scene_(assetManager_),
        physicsSystem_(scene_),
        vulkanWiringContainer_(window_, assetManager_, userInterface_, rendererSettings_),
        pickingSystem_(vulkanWiringContainer_.graphicsBackend().pickingBackend()),
        renderSystem_(vulkanWiringContainer_.graphicsBackend()),
        inputSystem_(window_),
        time_([&window] { return static_cast<float>(window.getTime()); }),
        engine_(window_,
                time_,
                userInterface_,
                assetManager_,
                inputSystem_,
                pickingSystem_,
                physicsSystem_,
                scene_,
                renderSystem_,
                rendererSettings_,
                vulkanWiringContainer_.graphicsBackend()) {}

    GameEngine& gameEngine() { return engine_; }

private:
    GameWindow& window_;
    RendererSettings rendererSettings_;
    UserInterface userInterface_;
    AssetStorage assetStorage_;
    AssetImporter assetImporter_;
    AssetManager assetManager_;
    Scene scene_;
    PhysicsSystem physicsSystem_;
    VulkanWiringContainer vulkanWiringContainer_;
    PickingSystem pickingSystem_;
    RenderSystem renderSystem_;
    InputSystem inputSystem_;
    Time time_;
    GameEngine engine_;
};
