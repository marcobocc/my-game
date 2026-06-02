#include "Imgui_Console.hpp"
#include <imgui.h>
#include <iomanip>
#include <sstream>

bool Imgui_Console::matchesFilters(const LogMessage& msg) const {
    if (enabledLevels_.find(msg.level) == enabledLevels_.end()) return false;
    if (!searchFilter_.empty()) {
        std::string lowerText = msg.text;
        std::string lowerSearch = searchFilter_;
        std::transform(
                lowerText.begin(), lowerText.end(), lowerText.begin(), [](unsigned char c) { return std::tolower(c); });
        std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        if (lowerText.find(lowerSearch) == std::string::npos) return false;
    }
    return true;
}

void Imgui_Console::setLevelFilter(LogLevel level, bool enabled) {
    if (enabled)
        enabledLevels_.insert(level);
    else
        enabledLevels_.erase(level);
}

void Imgui_Console::setSearchFilter(const std::string& search) { searchFilter_ = search; }

static std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Imgui_Console::draw() {
    if (!visible_) return;

    auto newMessages = responseBuffer_.consume();
    cachedMessages_.insert(cachedMessages_.end(), newMessages.begin(), newMessages.end());

    ImGui::Begin("Console", &visible_);
    bool windowActive = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    static bool wasWindowActive = false;
    bool windowJustFocused = windowActive && !wasWindowActive;
    wasWindowActive = windowActive;

    if (ImGui::BeginTable("console_controls", 4, ImGuiTableFlags_SizingFixedFit)) {
        bool debugEnabled = enabledLevels_.count(LogLevel::Debug) > 0;
        bool infoEnabled = enabledLevels_.count(LogLevel::Info) > 0;
        bool warnEnabled = enabledLevels_.count(LogLevel::Warn) > 0;
        bool errorEnabled = enabledLevels_.count(LogLevel::Error) > 0;

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Checkbox("Debug", &debugEnabled)) setLevelFilter(LogLevel::Debug, debugEnabled);
        ImGui::TableSetColumnIndex(1);
        if (ImGui::Checkbox("Info", &infoEnabled)) setLevelFilter(LogLevel::Info, infoEnabled);
        ImGui::TableSetColumnIndex(2);
        if (ImGui::Checkbox("Warn", &warnEnabled)) setLevelFilter(LogLevel::Warn, warnEnabled);
        ImGui::TableSetColumnIndex(3);
        if (ImGui::Checkbox("Error", &errorEnabled)) setLevelFilter(LogLevel::Error, errorEnabled);

        ImGui::EndTable();
    }

    static char searchBuf[256] = {0};
    if (ImGui::InputText("Search##console", searchBuf, sizeof(searchBuf))) setSearchFilter(searchBuf);

    float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    ImGui::BeginChild("scroll", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_None);
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x);

    for (const auto& entry: cachedMessages_) {
        if (!entry) continue;
        auto* logMsg = dynamic_cast<LogMessage*>(entry.get());
        if (logMsg) {
            if (!matchesFilters(*logMsg)) continue;
            std::string fullText = formatTimestamp(logMsg->timestamp) + " - " + logMsg->text;
            if (logMsg->level == LogLevel::Error) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", fullText.c_str());
            } else if (logMsg->level == LogLevel::Warn) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "%s", fullText.c_str());
            } else {
                ImGui::TextUnformatted(fullText.c_str());
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "> %s", entry->serialize().c_str());
        }
    }

    ImGui::PopTextWrapPos();
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetItemRectSize().y) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    static char commandBuf[256] = {0};
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::SetNextItemWidth(-1.0f);
    bool inputChanged =
            ImGui::InputText("##command", commandBuf, sizeof(commandBuf), ImGuiInputTextFlags_EnterReturnsTrue);
    if (inputChanged) {
        lastCommand_ = commandBuf;
        console_->submitCommand(commandBuf);
        commandBuf[0] = '\0';
        matchedCommands_.clear();
    } else if (ImGui::IsItemActive()) {
        std::string input = commandBuf;
        matchedCommands_.clear();
        if (!input.empty()) {
            for (const auto& cmd: console_->listCommands()) {
                if (cmd.substr(0, input.size()) == input) matchedCommands_.push_back(cmd);
            }
        }
    }
    if (windowJustFocused && !ImGui::IsItemActive()) ImGui::SetKeyboardFocusHere(-1);
    ImGui::PopStyleColor(3);

    if (!matchedCommands_.empty()) {
        ImVec2 inputPos = ImGui::GetItemRectMin();
        ImVec2 inputSize = ImGui::GetItemRectSize();
        ImGui::SetNextWindowPos(ImVec2(inputPos.x, inputPos.y + inputSize.y));
        ImGui::SetNextWindowSize(ImVec2(inputSize.x, 0));
        ImGui::Begin("##autocomplete",
                     nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_Tooltip);
        for (int i = 0; i < static_cast<int>(matchedCommands_.size()); ++i) {
            ImGui::PushID(i);
            if (ImGui::Selectable(matchedCommands_[i].c_str(), false)) {
                snprintf(commandBuf, sizeof(commandBuf), "%s ", matchedCommands_[i].c_str());
                matchedCommands_.clear();
            }
            ImGui::PopID();
        }
        ImGui::End();
    }

    ImGui::End();
}
