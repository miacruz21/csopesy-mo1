#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#ifdef TEST_MODE
inline constexpr bool kTestMode = true;
#else
inline constexpr bool kTestMode = false;
#endif

class ConfigManager {
public:
    bool      load(const std::string& filename);
    std::string get(const std::string& key)        const;
    uint64_t    get_long(const std::string& key)   const;
private:
    std::unordered_map<std::string,std::string> values;
};