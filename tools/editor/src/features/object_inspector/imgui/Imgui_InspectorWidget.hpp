#pragma once
#include <functional>
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
#include "../../../styling/ImguiStyling.hpp"
#include "../../../styling/InspectorProperty.hpp"

class Imgui_InspectorWidget {
public:
    virtual ~Imgui_InspectorWidget() = default;
    explicit Imgui_InspectorWidget(std::string title) : title_(std::move(title)) {}

    void draw(const char* contextId = nullptr, std::function<void()> onRemove = nullptr) {
        if (!ImGui::BeginChild(title_.c_str(), {0, 0}, ImGuiChildFlags_AutoResizeY)) {
            ImGui::EndChild();
            return;
        }
        drawHeader(contextId, onRemove);

        ImguiStyling::withBodyStyling([this] {
            ImGui::BeginChild(
                    "##Body", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);

            properties_.clear();
            buildProperties();
            for (auto& p: properties_)
                p->draw();

            ImGui::EndChild();
        });
        ImGui::EndChild();
    }

protected:
    const char* getTitle() const { return title_.c_str(); }

    // Subclasses populate properties_ by calling add() inside buildProperties().
    virtual void buildProperties() = 0;

    template<typename T, typename... Args>
    void add(Args&&... args) {
        properties_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void addRaw(std::unique_ptr<InspectorProperty> p) { properties_.emplace_back(std::move(p)); }

private:
    std::string title_;
    std::vector<std::unique_ptr<InspectorProperty>> properties_;

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
        ImGui::Button(title_.c_str(), {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y});
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        if (contextId) {
            ImguiStyling::withPopup(
                    contextId,
                    [onRemove] {
                        bool canRemove = onRemove != nullptr;
                        if (ImGui::MenuItem("Remove", nullptr, false, canRemove) && canRemove) onRemove();
                    },
                    true);
        }
        ImGui::EndChild();
    }
};
