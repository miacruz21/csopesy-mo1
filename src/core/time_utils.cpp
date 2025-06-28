#include "time_utils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace util {

std::string now_time()
{
    using std::chrono::system_clock;
    auto t = system_clock::to_time_t(system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

}
