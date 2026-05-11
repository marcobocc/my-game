#pragma once
#include <functional>
#include <imgui.h>
#include "../../ImguiStyling.hpp"

class ComponentContainer {
public:
    virtual ~ComponentContainer() = default;
    ComponentContainer(const char* title, int rows) : title_(title), rows_(rows) {}

    void draw(const char* contextId = nullptr, std::function<void()> onRemove = nullptr) {
        if (!ImGui::BeginChild(title_, {0, childHeight(rows_)}, false)) {
            ImGui::EndChild();
            return;
        }
        drawHeader(contextId, onRemove);

        ImguiStyling::withBodyStyling([this] {
            ImGui::BeginChild("##Body",
                              ImVec2(0, 0),
                              ImGuiChildFlags_AlwaysUseWindowPadding,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            drawBody();
            ImGui::EndChild();
        });
        ImGui::EndChild();
    }

protected:
    const char* getTitle() const { return title_; }
    virtual void drawBody() = 0;

private:
    const char* title_;
    int rows_;

    void drawHeader(const char* contextId = nullptr, std::function<void()> onRemove = nullptr) {
        constexpr float height = 20.0f;
        ImGui::BeginChild("##Header",
                          ImVec2(0, height),
                          ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::PushStyleColor(ImGuiCol_Button, {0.12f, 0.12f, 0.12f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.14f, 0.14f, 0.14f, 1.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.5f});
        ImGui::Button(title_, {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y});
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        if (contextId) {
            ImguiStyling::withPopup(
                    contextId,
                    [onRemove] {
                        if (ImGui::MenuItem("Remove")) {
                            onRemove();
                        }
                    },
                    true);
        }
        ImGui::EndChild();
    }

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y;
    }
};
