#pragma once
#include <nlohmann/json.hpp>

class IComponent {
public:
    virtual ~IComponent() = default;

    virtual std::string typeName() const = 0;
    virtual nlohmann::json serialize() const = 0;
    virtual std::unique_ptr<IComponent> clone() const = 0;
    virtual bool allowMultiple() const { return false; }
};
