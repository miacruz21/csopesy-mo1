#include "config_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <cstdint>
#include <stdexcept>

bool ConfigManager::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << "\n";
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        // Remove comments and trailing/leading whitespace
        size_t comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string key, value;
        iss >> key >> value;
        if (key.empty() || value.empty()) continue;

        if (key == "num-cpu") {
            int v = std::stoi(value);
            if (v < 1 || v > 128) {
                std::cerr << "num-cpu must be in [1, 128].\n";
                return false;
            }
            values["num-cpu"] = std::to_string(v);
        }
        else if (key == "scheduler") {
            if (value != "fcfs" && value != "rr") {
                std::cerr << "scheduler must be 'fcfs' or 'rr'.\n";
                return false;
            }
            values["scheduler"] = value;
        }
        else if (key == "quantum-cycles") {
            uint64_t v = std::stoull(value);
            if (v < 1 || v > UINT32_MAX) {
                std::cerr << "quantum-cycles must be in [1, 2^32].\n";
                return false;
            }
            values["quantum-cycles"] = std::to_string(v);
        }
        else if (key == "batch-process-freq") {
            uint64_t v = std::stoull(value);
            if (v < 1 || v > UINT32_MAX) {
                std::cerr << "batch-process-freq must be in [1, 2^32].\n";
                return false;
            }
            values["batch-process-freq"] = std::to_string(v);
        }
        else if (key == "min-ins") {
            uint64_t v = std::stoull(value);
            if (v < 1 || v > UINT32_MAX) {
                std::cerr << "min-ins must be in [1, 2^32].\n";
                return false;
            }
            values["min-ins"] = std::to_string(v);
        }
        else if (key == "max-ins") {
            uint64_t v = std::stoull(value);
            if (v < 1 || v > UINT32_MAX) {
                std::cerr << "max-ins must be in [1, 2^32].\n";
                return false;
            }
            values["max-ins"] = std::to_string(v);
        }
        else if (key == "delays-per-exec") {
            uint64_t v = std::stoull(value);
            if (v > UINT32_MAX) {
                std::cerr << "delays-per-exec must be in [0, 2^32].\n";
                return false;
            }
            values["delays-per-exec"] = std::to_string(v);
        }
        else {
            std::cerr << "Unknown config parameter: " << key << "\n";
            return false;
        }
    }
    file.close();
    return true;
}

std::string ConfigManager::get(const std::string& key) const {
    auto it = values.find(key);
    if (it == values.end())
        throw std::runtime_error("Missing config key: " + key);
    return it->second;
}

uint64_t ConfigManager::get_long(const std::string& key) const {
    return std::stoull(get(key));
}
