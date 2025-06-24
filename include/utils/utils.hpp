#pragma once
#include <string>
#include <iomanip>
#include <algorithm>

namespace utils {
    inline std::string truncate_text(const std::string& text, size_t max_length) {
        if (text.length() <= max_length) return text;
        return text.substr(0, max_length - 3) + "...";
    }

    inline std::string to_lower(const std::string& str) {
        std::string lower_str = str;
        std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                      [](unsigned char c){ return std::tolower(c); });
        return lower_str;
    }

    inline bool is_numeric(const std::string& str) {
        return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
    }
}