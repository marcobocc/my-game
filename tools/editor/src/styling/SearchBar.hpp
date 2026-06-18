#pragma once
#include <imgui.h>
#include <string>

// Draws a full-width search input inside a styled child strip.
// buf/bufSize: caller-owned char buffer. Returns true if text changed.
static inline bool drawSearchBar(const char* id, char* buf, size_t bufSize, const char* hint = "Search...") {
    constexpr float height = 24.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(45, 45, 45, 220));
    ImGui::BeginChild(id,
                      ImVec2(0, height),
                      ImGuiChildFlags_AlwaysUseWindowPadding,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(38, 38, 38, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(45, 45, 45, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(52, 52, 52, 255));
    const std::string inputId = std::string("##") + id;
    bool changed = ImGui::InputTextWithHint(inputId.c_str(), hint, buf, bufSize);
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    return changed;
}
