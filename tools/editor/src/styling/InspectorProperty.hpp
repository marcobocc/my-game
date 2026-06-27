#pragma once
#include <functional>
#include <glm/glm.hpp>
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

// Base class for all inspector property rows.
class InspectorProperty {
public:
    virtual ~InspectorProperty() = default;
    virtual void draw() = 0;

protected:
    static constexpr float kLabelWidth = 110.0f;

    template<typename Fn>
    static void row(const char* label, Fn&& fn) {
        ImGui::PushID(label);
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, kLabelWidth);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        fn();
        ImGui::Columns(1);
        ImGui::PopID();
    }

    template<typename Fn>
    static void rowNoWidth(const char* label, Fn&& fn) {
        ImGui::PushID(label);
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, kLabelWidth);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        fn();
        ImGui::Columns(1);
        ImGui::PopID();
    }
};

// ---- Separator / Spacing ------------------------------------------------

class SeparatorProperty : public InspectorProperty {
public:
    void draw() override {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
};

class SpacingProperty : public InspectorProperty {
public:
    void draw() override { ImGui::Spacing(); }
};

// ---- Label (read-only text) ---------------------------------------------

class LabelProperty : public InspectorProperty {
public:
    LabelProperty(std::string label, std::function<std::string()> getValue) :
        label_(std::move(label)),
        getValue_(std::move(getValue)) {}

    void draw() override {
        row(label_.c_str(), [&] {
            auto v = getValue_();
            ImGui::TextUnformatted(v.c_str());
        });
    }

private:
    std::string label_;
    std::function<std::string()> getValue_;
};

// ---- FloatLabel (read-only formatted float) -----------------------------

class FloatLabelProperty : public InspectorProperty {
public:
    FloatLabelProperty(std::string label, std::function<float()> getValue, const char* fmt = "%.3f") :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        fmt_(fmt) {}

    void draw() override {
        row(label_.c_str(), [&] { ImGui::Text(fmt_, getValue_()); });
    }

private:
    std::string label_;
    std::function<float()> getValue_;
    const char* fmt_;
};

// ---- Checkbox -----------------------------------------------------------

class CheckboxProperty : public InspectorProperty {
public:
    CheckboxProperty(std::string label, std::function<bool()> getValue, std::function<void(bool)> onChange) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onChange_(std::move(onChange)) {}

    void draw() override {
        rowNoWidth(label_.c_str(), [&] {
            bool v = getValue_();
            if (ImGui::Checkbox("##cb", &v)) onChange_(v);
        });
    }

private:
    std::string label_;
    std::function<bool()> getValue_;
    std::function<void(bool)> onChange_;
};

// ---- Shared spinbox drawing (styled DragFloat, like DraggableDoubleSpinBox) -

struct SpinboxStyle {
    static void push() {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.21f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.26f, 0.32f, 1.0f));
    }
    static void pop() {
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
    }
};

// Draws a single styled DragFloat. Returns true if value changed.
static inline bool drawSpinbox(const char* id, float& v, float speed, float min, float max, const char* fmt = "%.3f") {
    SpinboxStyle::push();
    bool changed = ImGui::DragFloat(id, &v, speed, min, max, fmt);
    SpinboxStyle::pop();
    return changed;
}

// ---- DragFloat (with optional undo-commit callback on deactivation) ------

class DragFloatProperty : public InspectorProperty {
public:
    DragFloatProperty(std::string label,
                      std::function<float()> getValue,
                      std::function<void(float)> onChange,
                      float speed = 0.1f,
                      float min = 0.0f,
                      float max = 0.0f,
                      std::function<void()> onDeactivated = nullptr) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onChange_(std::move(onChange)),
        speed_(speed),
        min_(min),
        max_(max),
        onDeactivated_(std::move(onDeactivated)) {}

    void draw() override {
        row(label_.c_str(), [&] {
            float v = getValue_();
            if (drawSpinbox("##v", v, speed_, min_, max_)) onChange_(v);
            if (onDeactivated_ && ImGui::IsItemDeactivatedAfterEdit()) onDeactivated_();
        });
    }

private:
    std::string label_;
    std::function<float()> getValue_;
    std::function<void(float)> onChange_;
    float speed_, min_, max_;
    std::function<void()> onDeactivated_;
};

// ---- SliderInt ----------------------------------------------------------

class SliderIntProperty : public InspectorProperty {
public:
    SliderIntProperty(std::string label,
                      std::function<int()> getValue,
                      std::function<void(int)> onChange,
                      int min = 0,
                      int max = 100) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onChange_(std::move(onChange)),
        min_(min),
        max_(max) {}

    void draw() override {
        row(label_.c_str(), [&] {
            int v = getValue_();
            if (ImGui::SliderInt("##v", &v, min_, max_)) onChange_(v);
        });
    }

private:
    std::string label_;
    std::function<int()> getValue_;
    std::function<void(int)> onChange_;
    int min_, max_;
};

// ---- SliderFloat --------------------------------------------------------

class SliderFloatProperty : public InspectorProperty {
public:
    SliderFloatProperty(std::string label,
                        std::function<float()> getValue,
                        std::function<void(float)> onChange,
                        float min = 0.0f,
                        float max = 1.0f) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onChange_(std::move(onChange)),
        min_(min),
        max_(max) {}

    void draw() override {
        row(label_.c_str(), [&] {
            float v = getValue_();
            if (ImGui::SliderFloat("##v", &v, min_, max_)) onChange_(v);
        });
    }

private:
    std::string label_;
    std::function<float()> getValue_;
    std::function<void(float)> onChange_;
    float min_, max_;
};

// ---- ColorEdit3 ---------------------------------------------------------

class ColorEdit3Property : public InspectorProperty {
public:
    ColorEdit3Property(std::string label,
                       std::function<glm::vec3()> getValue,
                       std::function<void(glm::vec3)> onChange) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onChange_(std::move(onChange)) {}

    void draw() override {
        row(label_.c_str(), [&] {
            auto c = getValue_();
            float col[3] = {c.r, c.g, c.b};
            if (ImGui::ColorEdit3("##v", col)) onChange_({col[0], col[1], col[2]});
        });
    }

private:
    std::string label_;
    std::function<glm::vec3()> getValue_;
    std::function<void(glm::vec3)> onChange_;
};

// ---- ColorEdit4 (two-column label layout, optional undo callbacks) ------

class ColorEdit4Property : public InspectorProperty {
public:
    ColorEdit4Property(std::string label,
                       std::function<glm::vec4()> getValue,
                       std::function<void(glm::vec4)> onChange,
                       std::function<void()> onActivated = nullptr,
                       std::function<void()> onDeactivated = nullptr) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onChange_(std::move(onChange)),
        onActivated_(std::move(onActivated)),
        onDeactivated_(std::move(onDeactivated)) {}

    void draw() override {
        row(label_.c_str(), [&] {
            auto c = getValue_();
            float col[4] = {c.r, c.g, c.b, c.a};
            if (ImGui::ColorEdit4("##v", col)) onChange_({col[0], col[1], col[2], col[3]});
            if (onActivated_ && ImGui::IsItemActivated()) onActivated_();
            if (onDeactivated_ && ImGui::IsItemDeactivatedAfterEdit()) onDeactivated_();
        });
    }

private:
    std::string label_;
    std::function<glm::vec4()> getValue_;
    std::function<void(glm::vec4)> onChange_;
    std::function<void()> onActivated_;
    std::function<void()> onDeactivated_;
};

// ---- Combo (enum selector) ----------------------------------------------

class ComboProperty : public InspectorProperty {
public:
    ComboProperty(std::string label,
                  std::vector<std::string> items,
                  std::function<int()> getIndex,
                  std::function<void(int)> onChange) :
        label_(std::move(label)),
        items_(std::move(items)),
        getIndex_(std::move(getIndex)),
        onChange_(std::move(onChange)) {}

    void draw() override {
        row(label_.c_str(), [&] {
            int idx = getIndex_();
            std::string joined;
            for (auto& s: items_) {
                joined += s;
                joined += '\0';
            }
            if (ImGui::Combo("##v", &idx, joined.c_str())) onChange_(idx);
        });
    }

private:
    std::string label_;
    std::vector<std::string> items_;
    std::function<int()> getIndex_;
    std::function<void(int)> onChange_;
};

// ---- InputText (read-only with optional drag-drop) ----------------------

class InputTextProperty : public InspectorProperty {
public:
    InputTextProperty(std::string label,
                      std::function<std::string()> getValue,
                      std::string dragDropType = "",
                      std::function<void(const char*)> onDrop = nullptr) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        dragDropType_(std::move(dragDropType)),
        onDrop_(std::move(onDrop)) {}

    void draw() override {
        row(label_.c_str(), [&] {
            auto v = getValue_();
            ImGui::InputText("##v", const_cast<char*>(v.c_str()), v.size() + 1, ImGuiInputTextFlags_ReadOnly);
            if (!dragDropType_.empty() && onDrop_ && ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType_.c_str()))
                    onDrop_(static_cast<const char*>(payload->Data));
                ImGui::EndDragDropTarget();
            }
        });
    }

private:
    std::string label_;
    std::function<std::string()> getValue_;
    std::string dragDropType_;
    std::function<void(const char*)> onDrop_;
};

// ---- InputTextEditable (editable with enter-to-commit and drag-drop) ----

class InputTextEditableProperty : public InspectorProperty {
public:
    InputTextEditableProperty(std::string label,
                              std::array<char, 256>& buf,
                              std::function<void()> onCommit,
                              std::string dragDropType = "",
                              std::function<void(const char*)> onDrop = nullptr) :
        label_(std::move(label)),
        buf_(buf),
        onCommit_(std::move(onCommit)),
        dragDropType_(std::move(dragDropType)),
        onDrop_(std::move(onDrop)) {}

    void draw() override {
        row(label_.c_str(), [&] {
            if (ImGui::InputText("##v", buf_.data(), buf_.size(), ImGuiInputTextFlags_EnterReturnsTrue)) onCommit_();
            if (!dragDropType_.empty() && onDrop_ && ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDropType_.c_str()))
                    onDrop_(static_cast<const char*>(payload->Data));
                ImGui::EndDragDropTarget();
            }
        });
    }

private:
    std::string label_;
    std::array<char, 256>& buf_;
    std::function<void()> onCommit_;
    std::string dragDropType_;
    std::function<void(const char*)> onDrop_;
};

// ---- Vec3 (three styled spinboxes, optional undo tracking) --------------

class Vec3Property : public InspectorProperty {
public:
    Vec3Property(std::string label,
                 std::function<glm::vec3()> getValue,
                 std::function<void(glm::vec3)> onChange,
                 float speed = 0.01f,
                 std::function<void()> onActivated = nullptr,
                 std::function<void()> onDeactivated = nullptr) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onChange_(std::move(onChange)),
        speed_(speed),
        onActivated_(std::move(onActivated)),
        onDeactivated_(std::move(onDeactivated)) {}

    void draw() override {
        rowNoWidth(label_.c_str(), [&] {
            auto v = getValue_();
            float w = (ImGui::GetContentRegionAvail().x - 8.0f) / 3.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
            bool changed = false;
            ImGui::SetNextItemWidth(w);
            if (drawSpinbox("##x", v.x, speed_, 0, 0, "X: %.2f")) {
                onChange_(v);
                changed = true;
            }
            trackItem();
            ImGui::SameLine();
            ImGui::SetNextItemWidth(w);
            if (drawSpinbox("##y", v.y, speed_, 0, 0, "Y: %.2f")) {
                onChange_(v);
                changed = true;
            }
            trackItem();
            ImGui::SameLine();
            ImGui::SetNextItemWidth(w);
            if (drawSpinbox("##z", v.z, speed_, 0, 0, "Z: %.2f")) {
                onChange_(v);
                changed = true;
            }
            trackItem();
            ImGui::PopStyleVar();
            (void) changed;
        });
    }

private:
    std::string label_;
    std::function<glm::vec3()> getValue_;
    std::function<void(glm::vec3)> onChange_;
    float speed_;
    std::function<void()> onActivated_;
    std::function<void()> onDeactivated_;

    void trackItem() {
        if (onActivated_ && ImGui::IsItemActivated()) onActivated_();
        if (onDeactivated_ && ImGui::IsItemDeactivatedAfterEdit()) onDeactivated_();
    }
};

// ---- Vec2 (two styled spinboxes) ----------------------------------------

class Vec2Property : public InspectorProperty {
public:
    Vec2Property(std::string label,
                 std::function<glm::vec2()> getValue,
                 std::function<void(glm::vec2)> onChange,
                 float speed = 0.01f) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onChange_(std::move(onChange)),
        speed_(speed) {}

    void draw() override {
        rowNoWidth(label_.c_str(), [&] {
            auto v = getValue_();
            float w = (ImGui::GetContentRegionAvail().x - 4.0f) / 2.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
            bool changed = false;
            ImGui::SetNextItemWidth(w);
            changed |= drawSpinbox("##x", v.x, speed_, 0, 0, "X: %.3f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(w);
            changed |= drawSpinbox("##y", v.y, speed_, 0, 0, "Y: %.3f");
            ImGui::PopStyleVar();
            if (changed) onChange_(v);
        });
    }

private:
    std::string label_;
    std::function<glm::vec2()> getValue_;
    std::function<void(glm::vec2)> onChange_;
    float speed_;
};

// ---- EntityProperty (read-only entity ID with HIERARCHY_ENTITY drag-drop) -

class EntityProperty : public InspectorProperty {
public:
    EntityProperty(std::string label, std::function<uint64_t()> getValue, std::function<void(uint64_t)> onDrop) :
        label_(std::move(label)),
        getValue_(std::move(getValue)),
        onDrop_(std::move(onDrop)) {}

    void draw() override {
        row(label_.c_str(), [&] {
            uint64_t id = getValue_();
            std::string text = (id == std::numeric_limits<uint64_t>::max()) ? "(none)" : std::to_string(id);
            ImGui::InputText("##v", const_cast<char*>(text.c_str()), text.size() + 1, ImGuiInputTextFlags_ReadOnly);
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
                    uint64_t dropped = *static_cast<const uint64_t*>(payload->Data);
                    onDrop_(dropped);
                }
                ImGui::EndDragDropTarget();
            }
        });
    }

private:
    std::string label_;
    std::function<uint64_t()> getValue_;
    std::function<void(uint64_t)> onDrop_;
};

// ---- DisabledGroup (wraps child properties in BeginDisabled/EndDisabled) -

class DisabledGroupProperty : public InspectorProperty {
public:
    explicit DisabledGroupProperty(std::vector<std::unique_ptr<InspectorProperty>> children) :
        children_(std::move(children)) {}

    void draw() override {
        ImGui::BeginDisabled();
        for (auto& p: children_)
            p->draw();
        ImGui::EndDisabled();
    }

private:
    std::vector<std::unique_ptr<InspectorProperty>> children_;
};
