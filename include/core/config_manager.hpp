#pragma once
#include <string>
#include <unordered_map>
#include <stdexcept>

class ConfigManager {
private:
    std::unordered_map<std::string, std::string> config;

public:
    void load(const std::string& filename);
    std::string get(const std::string& key) const;
    int get_int(const std::string& key) const;
    bool has(const std::string& key) const;
};