#pragma once
#include <imgui.h>
#include <vulkan/vulkan.h>
#include "../../../../../engine/GameEngine.hpp"
#include "../../../../../engine/data/RenderTargetHandle.hpp"
#include "systems/ui/ImguiWidget.hpp"

class RenderingController;

class RenderTargetPreview : public ImguiWidget {
public:
    RenderTargetPreview(GameEngine& engine, RenderingController& renderingController, RenderTargetHandle handle) :
        engine_(engine),
        renderingController_(renderingController),
        handle_(handle) {}

    void draw() const override {
        if (!handle_.isValid()) return;
        VkDescriptorSet texId = renderingController_.getRenderTargetImGuiId(handle_);
        if (texId == VK_NULL_HANDLE) return;
        ImGui::SetNextWindowSize({300, 300}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Preview")) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float size = std::min(avail.x, avail.y);
            ImGui::Image(reinterpret_cast<ImTextureID>(texId), {size, size});
        }
        ImGui::End();
    }

private:
    GameEngine& engine_;
    RenderingController& renderingController_;
    RenderTargetHandle handle_;
};
