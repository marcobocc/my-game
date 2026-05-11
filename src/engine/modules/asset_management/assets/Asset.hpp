#pragma once
#include <string>

class Asset {
public:
    explicit Asset(const std::string& name) : name(name) {}

    std::string name;
};
