#pragma once
#include <string>

class CPUResource {
public:
    explicit CPUResource(const std::string& name) : name(name) {}
    virtual ~CPUResource() = default;

    std::string name;
};
