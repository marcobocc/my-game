#pragma once
#include <nlohmann/json.hpp>

class IComponent {
public:
    virtual ~IComponent() = default;

    virtual std::string typeName() const = 0;
    virtual nlohmann::json serialize() const = 0;
};
