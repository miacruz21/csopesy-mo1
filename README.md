# CSOPESY MO1 – CPU OS Emulator  
*Cruz, Mia Bernice  •  Del Mundo, Rhamon Khayle*  

---

## 1 . Overview
A console-driven emulator that demonstrates process scheduling (FCFS / Round-Robin), per-tick CPU-utilisation tracking, and live per-process logging.  
All code is standard C++17.

---

## 2 . Build & Run (Windows · MinGW / Git-Bash)

```bash
# clone
git clone https://github.com/miacruz21/csopesy-mo1.git
cd csopesy-mo1

# Compile
g++ -std=c++17 -Isrc ^
    src/main.cpp src/cli/console.cpp ^
    src/core/config_manager.cpp src/core/cpu_utilization.cpp ^
    src/core/instruction.cpp src/core/process.cpp ^
    src/core/process_manager.cpp src/core/scheduler.cpp ^
    src/common/time_utils.cpp ^
    -o csopesy.exe

# Place a valid config.txt next to the exe
./csopesy.exe
```

## 3. Entry Point
File: `src/main.cpp`
Function:
```bash
#include "cli/console.h"

int main() {
    auto app = std::make_unique<Console>();
    app->run();
}
```

## 4. Primary Commands



