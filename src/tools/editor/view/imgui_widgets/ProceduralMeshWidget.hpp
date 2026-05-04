#pragma once
#include <imgui.h>
#include <string>
#include "../../../../engine/GameEngine.hpp"
#include "../../controller/SceneMutationsController.hpp"

class ProceduralMeshWidget {
public:
    ProceduralMeshWidget(GameEngine& engine, SceneMutationsController& mutations) :
        engine_(engine),
        mutations_(mutations),
        sphereResolution_(5) {}

    void draw() {
        if (!ImGui::CollapsingHeader("Procedural Meshes")) return;

        ImGui::Indent();

        ImGui::Text("Create Sphere");
        ImGui::SliderInt("Resolution", reinterpret_cast<int*>(&sphereResolution_), 4, 16);

        if (ImGui::Button("Create Procedural Sphere", ImVec2(-1, 0))) {
            createProceduralSphere();
        }

        ImGui::Unindent();
    }

private:
    GameEngine& engine_;
    SceneMutationsController& mutations_;
    uint32_t sphereResolution_;

    void createProceduralSphere() {
        std::string meshName = "_PROCEDURAL_SPHERE_" + std::to_string(sphereResolution_);
        try {
            auto objectId = engine_.createMesh(meshName, {});
        } catch (const std::exception&) {
        }
    }
};
