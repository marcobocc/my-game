#pragma once
#include <cctype>
#include <string>

inline std::string toTitleCase(std::string s) {
    bool capitalize = true;
    for (char& c: s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            capitalize = true;
        } else if (capitalize) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalize = false;
        } else {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return s;
}
