#pragma once
#include <mutex>

#define LOG(msg) do { \
    std::lock_guard<std::mutex> guard(logger_mutex); \
    std::cout << "[LOG] " << msg << std::endl; \
} while (0)
extern std::mutex logger_mutex;