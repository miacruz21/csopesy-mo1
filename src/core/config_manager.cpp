#include "core/config_manager.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <ranges>
#include <algorithm>

void ConfigManager::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filename);
    }

    std::unordered_map<std::string, std::string> temp_config;
    std::string line;
    
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        std::istringstream iss(line);
        std::string key, value;
        if (iss >> key && std::getline(iss >> std::ws, value)) {
            // Remove surrounding quotes if present
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            temp_config[key] = value;
        }
    }

    // Validate required parameters
    const std::vector<std::string> required_params = {
        "num-cpu", "scheduler", "quantum-cycles", 
        "batch-process-freq", "min-ins", "max-ins", "delays-per-exec"
    };

    for (const auto& param : required_params) {
        if (temp_config.find(param) == temp_config.end()) {
            throw std::runtime_error("Missing required config parameter: " + param);
        }
    }

    // Validate scheduler type
    std::string scheduler_type = temp_config["scheduler"];
    std::transform(scheduler_type.begin(), scheduler_type.end(), scheduler_type.begin(), ::tolower);
    if (scheduler_type != "fcfs" && scheduler_type != "rr") {
        throw std::runtime_error("Invalid scheduler type. Must be 'fcfs' or 'rr'");
    }

   // Validate numeric parameters
    try {
        int num_cpu = std::stoi(temp_config["num-cpu"]);
        if (num_cpu < 1 || num_cpu > 128) {
            throw std::runtime_error("num-cpu must be between 1 and 128");
    }

    int batch_freq = std::stoi(temp_config["batch-process-freq"]);
    if (batch_freq < 1) {
        throw std::runtime_error("batch-process-freq must be at least 1");
    }

    int min_ins = std::stoi(temp_config["min-ins"]);
    if (min_ins < 1) {
        throw std::runtime_error("min-ins must be at least 1");
    }

    int max_ins = std::stoi(temp_config["max-ins"]);
    if (max_ins < 1) {
        throw std::runtime_error("max-ins must be at least 1");
    }
    if (max_ins < min_ins) {
        throw std::runtime_error("max-ins cannot be less than min-ins");
    }

    int delays = std::stoi(temp_config["delays-per-exec"]);
    if (delays < 0) {
        throw std::runtime_error("delays-per-exec cannot be negative");
    }

    if (scheduler_type == "rr") {
        int quantum = std::stoi(temp_config["quantum-cycles"]);
        if (quantum < 1) {
            throw std::runtime_error("quantum-cycles must be at least 1 for RR scheduler");
        }
    }
        // Similar validation for other numeric parameters...
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Invalid numeric value in config");
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Numeric value out of range in config");
    }

    // If all validations pass, update the config
    config = std::move(temp_config);
}

std::string ConfigManager::get(const std::string& key) const {
    auto it = config.find(key);
    if (it == config.end()) {
        throw std::runtime_error("Config key not found: " + key);
    }
    return it->second;
}

int ConfigManager::get_int(const std::string& key) const {
    return std::stoi(get(key));
}

bool ConfigManager::has(const std::string& key) const {
    return config.find(key) != config.end();
}