#include "config_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <cstdint>
#include <stdexcept>

bool ConfigManager::load(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << '\n';
        return false;
    }

    auto trim = [](std::string& s)
    {
        if (auto hash = s.find('#'); hash != std::string::npos) s.erase(hash);
        auto first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) { s.clear(); return; }
        auto last  = s.find_last_not_of(" \t\r\n");
        s = s.substr(first, last - first + 1);
    };

    std::string line;
    while (std::getline(file, line)) {
        trim(line);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;
        if (key.empty() || value.empty()) continue;

        if (value.size() >= 2 && value.front()=='"' && value.back()=='"')
            value = value.substr(1, value.size()-2);

        // ---------- per-key validation ----------
        if (key == "num-cpu") {
            long v = std::stol(value);
            if (v < 1 || v > 128) { std::cerr << "num-cpu must be [1,128]\n"; return false; }
        }
        else if (key == "scheduler") {
            if (value != "fcfs" && value != "rr") {
                std::cerr << "scheduler must be fcfs or rr\n"; return false;
            }
        }
        else if (key == "quantum-cycles") {
            unsigned long long v = std::stoull(value);
            if (v == 0 || v > UINT32_MAX) {
                std::cerr << "quantum-cycles out of range\n"; return false;
            }
        }
        else if (key == "batch-process-freq") {
            unsigned long long v = std::stoull(value);
            if (v == 0 || v > UINT32_MAX) {
                std::cerr << "batch-process-freq out of range\n"; return false;
            }
        }
        else if (key == "min-ins") {
            unsigned long long v = std::stoull(value);
            if (v == 0 || v > UINT32_MAX) {
                std::cerr << "min-ins out of range\n"; return false;
            }
        }
        else if (key == "max-ins") {
            unsigned long long v = std::stoull(value);
            if (v == 0 || v > UINT32_MAX) {
                std::cerr << "max-ins out of range\n"; return false;
            }
        }
        else if (key == "delays-per-exec") {
            unsigned long long v = std::stoull(value);     // 0 -- 2³²-1 allowed
            if (v > UINT32_MAX) {
                std::cerr << "delays-per-exec out of range\n"; return false;
            }
        }
        else {
            std::cerr << "Unknown config parameter: " << key << '\n';
            return false;
        }

        values[key] = value;
    }
    file.close();

    // -------- cross-field checks ----------
    const char* req[] = { "num-cpu","scheduler","quantum-cycles",
                          "batch-process-freq","min-ins","max-ins","delays-per-exec" };
    for (auto k : req)
        if (values.count(k)==0) {
            std::cerr << "Missing config key: " << k << '\n'; return false;
        }

    auto mi = std::stoul(values["min-ins"]);
    auto ma = std::stoul(values["max-ins"]);
    if (mi > ma) {
        std::cerr << "min-ins must be ≤ max-ins\n"; return false;
    }
    return true;
}

std::string ConfigManager::get(const std::string &key) const
{
    auto it = values.find(key);
    if (it == values.end())
        throw std::runtime_error("Missing config key: " + key);
    return it->second;
}

uint64_t ConfigManager::get_long(const std::string &key) const
{
    return std::stoull(get(key));
}
