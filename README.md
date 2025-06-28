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

`initialize`:    Parse config.txt, create `ProcessManager`, start scheduler

`screen -s <name>`:    Spawn & attach to a new process screen

`screen -r <name>`:    Re-attach to an existing running process

`screen -ls`:    	Utilisation + running/finished tables

`scheduler-start` / `scheduler/stop`:    Toggle automatic batch-process thread

`report-uti`:    	Print and append CPU-utilisation block to csopesy-log.txt

`help`:    Brief command list

`exit`:    Shutdown


## 5. Project Layout

```bash
src/
 ├── cli/console.{h,cpp}       ← UI, command loop
 ├── core/
 │    ├── process.{h,cpp}      ← code[], pc, vars, per-tick logging
 │    ├── process_manager.{h,cpp}
 │    ├── scheduler.{h,cpp}    ← FCFS & RR
 │    ├── instruction.{h,cpp}  ← PRINT, DECL, ADD, SUB, SLEEP, FOR
 │    ├── config_manager.{h,cpp}
 │    └── cpu_utilization.{h,cpp}
 ├── common/time_utils.{h,cpp}
 └── main.cpp                  ← entry

logs/             ← generated p<N>.txt per process  
csopesy-log.txt   ← utilisation reports
```

## 6. Authors

Mia Bernice Cruz (S13)


Rhamon Khayle Del Mundo (S14)




