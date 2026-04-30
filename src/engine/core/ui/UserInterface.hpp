#pragma once
#include <memory>
#include <vector>
#include "core/ui/ImguiWidget.hpp"

class UserInterface {
public:
    UserInterface() = default;

    const auto& widgets() const { return widgets_; }

    template<typename T, typename... Args>
    T* emplace(Args&&... args) {
        static_assert(std::is_base_of_v<ImguiWidget, T>, "T must derive from ImguiWidget");
        widgets_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return static_cast<T*>(widgets_.back().get());
    }

private:
    std::vector<std::unique_ptr<ImguiWidget>> widgets_{};
};
