# ERL-HUB — Emergency Relief Logistics Hub

**Subject:** Object-Oriented Programming Using C++ (CIC-211)  
**Project option:** 2 — Post-Earthquake Multi-Agency Emergency Relief Logistics Hub  
**Language:** C++17

## 1. Introduction

ERL-HUB is a console-based prototype for coordinating relief supplies and transport after a major earthquake. It maintains depot stock, assigns relief cargo to different vehicle types, prioritizes missions, checks route and vehicle constraints, and records successful dispatches in a persistent log.

The program models four transport types:
- Ground convoy truck
- Aerial medical transport / helicopter
- Search-and-rescue quadcopter drone
- Amphibious medical hybrid vehicle

The sample data is illustrative. This is an academic simulation, not a real emergency dispatch or navigation system.

## 2. Main functions

1. Create a regional depot and stock it with ration kits, trauma medicine, clean water, and thermal blankets.
2. Maintain a heterogeneous fleet through base-class pointers.
3. Sort and enqueue missions by priority (priority 1 is most urgent).
4. Allocate cargo using overloaded supply-allocation functions.
5. Check payload, fuel reserve, and damaged-route constraints.
6. Track dispatched tonnage and operational vehicle count.
7. Save transit audit records to `transit_log.bin` and verify each appended record.
8. Demonstrate custom exceptions, deep-copy dynamic memory, stream operators, and STL algorithms.

## 3. Build and run

### Linux / macOS (g++)

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp -o erl_hub
./erl_hub
```

### Windows (MinGW g++)

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp -o erl_hub.exe
erl_hub.exe
```

The program creates or appends to `transit_log.bin` in the current working directory. Remove that file before a fresh demonstration run if you want a clean log.

## 4. Project file

- `main.cpp` — complete source code. It is intentionally kept in one translation unit to make compiling and demonstrating the assignment straightforward.
- `PROJECT_REPORT.md` — project documentation, OOPS concept mapping, and testing notes.

## 5. Important note

The code uses C++17 because it uses `std::filesystem` for restoring the previous log-file length if a write or verification fails. Use a compiler with C++17 support.
