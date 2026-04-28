#pragma once
#include <string>

class Asset {
public:
    explicit Asset(const std::string& name) : name(name) {}
    const std::string& getName() const { return name; }

private:
    std::string name;
};
