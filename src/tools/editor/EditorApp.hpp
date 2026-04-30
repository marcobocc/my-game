#pragma once
#include "GameEngineWiringContainer.hpp"
#include "controllers/EditorUIController.hpp"
#include "controllers/OrbitCameraController.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "data/settings/RendererSettings.hpp"
#include "ui/HierarchyPanel.hpp"
#include "ui/InspectorPanel.hpp"

class EditorApp {
public:
    EditorApp(unsigned int windowWidth, unsigned int windowHeight, const std::filesystem::path& assetsPath) :
        window_(windowWidth, windowHeight, "Editor"),
        wiringContainer_(window_, assetsPath),
        engine_(wiringContainer_.gameEngine()),
        controller_(engine_),
        camera_(engine_) {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport(computeSceneViewport(w, h));
        window_.onWindowResize([this](int newW, int newH, int, int) {
            SceneViewport sv = computeSceneViewport(newW, newH);
            window_.setSceneViewport(sv);
            if (sv.width > 0 && sv.height > 0)
                camera_.setAspect(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        });
        setupScene();
    }

    void run() {
        engine_.run([this](double deltaTime) { update(deltaTime); });
    }

private:
    GameWindow window_;
    RendererSettings rendererSettings_;
    GameEngineWiringContainer wiringContainer_;
    GameEngine& engine_;
    EditorUIController controller_;
    OrbitCameraController camera_;

    bool wasLeftDown_ = false;

    static SceneViewport computeSceneViewport(int w, int h) {
        int left = static_cast<int>(static_cast<float>(w) * HierarchyPanel::PANEL_WIDTH_RATIO);
        int right = static_cast<int>(static_cast<float>(w) * (1.0f - InspectorPanel::PANEL_WIDTH_RATIO));
        return {left, 0, right - left, h};
    }

    void setupScene() {
        SceneViewport sv = window_.getSceneViewport();
        if (sv.width > 0 && sv.height > 0)
            camera_.setAspect(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        engine_.createCube({});
        engine_.enableWorldGrid();
    }

    void update(double deltaTime) {
        auto [mouseX, mouseY] = engine_.getMousePosition();

        bool ctrlHeld = engine_.isKeyDown(GLFW_KEY_LEFT_CONTROL) || engine_.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
        bool shiftHeld = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT) || engine_.isKeyDown(GLFW_KEY_RIGHT_SHIFT);
        if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_S) && controller_.scenePath)
            controller_.saveScene(*controller_.scenePath);
        if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_Z)) {
            if (shiftHeld)
                controller_.mutations.undoHistory().redo();
            else
                controller_.mutations.undoHistory().undo();
        }

        if (engine_.isKeyPressed(GLFW_KEY_G)) engine_.toggleWorldGrid();
        if (engine_.isKeyPressed(GLFW_KEY_L)) engine_.toggleLighting();
        if (engine_.isKeyPressed(GLFW_KEY_B)) controller_.gizmos.toggleBVH();

        bool leftDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        if (leftDown && !wasLeftDown_) {
            auto [scaleX, scaleY] = window_.getContentScale();
            engine_.requestPick(static_cast<uint32_t>(mouseX * scaleX), static_cast<uint32_t>(mouseY * scaleY));
        }
        wasLeftDown_ = leftDown;

        if (auto picked = engine_.getPickResult())
            controller_.selectedObjectId = (controller_.selectedObjectId == picked) ? std::nullopt : std::move(picked);

        if (controller_.selectedObjectId) {
            auto& obj = engine_.getObject(*controller_.selectedObjectId);
            if (obj.has<Renderer>() && obj.has<Transform>())
                engine_.drawObjectOutline(obj.get<Renderer>(), obj.get<Transform>(), *controller_.selectedObjectId);
        }

        controller_.gizmos.setSelectedObject(controller_.selectedObjectId);
        controller_.gizmos.draw();
        camera_.update(deltaTime);
    }
};
