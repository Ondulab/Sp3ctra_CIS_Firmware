1) Scope and Priority
- Scope: Applies to this repository. Complements any global rules you may have.
- Priority order when conflicts arise:
  1. System safety and stability
  2. Real-time (RT) embedded constraints
  3. Code conventions and quality
  4. Style and formatting
- Rules vs Requirements: These are implementation rules, not product requirements.

2) Language Policy
- Assistant conversation: French (user preference), but these rules are authored in English upon request.
- Code (C/C++):
  - Identifiers (functions, variables, files) must be clear English.
  - All comments, docstrings, logs, and error messages must be idiomatic English.
  - No French text is allowed in code under any circumstances.
- Formal documentation (README, guides, docs/): default French, unless explicitly requested otherwise.
- Commit messages: Conventional Commits, English only.
  - Examples:
    - feat(cis): add MDMA optimization for CIS data transfer
    - fix(udp): handle connection timeout on STM32H7
    - refactor(core): extract FreeRTOS task management

3) Real-Time (RT) Embedded Constraints
- Critical paths (ISR, DMA callbacks, high-frequency tasks):
  - Forbidden: dynamic allocation, locks (mutex), blocking I/O, logging/printf, C++ exceptions.
  - Allowed: bounded atomic operations, lock-free queues/rings, preallocated buffers, O(1) operations.
- Time budget (measurable): ISR execution time should not exceed 10% of interrupt period; FreeRTOS tasks must respect their deadlines.
- Memory: preallocate at startup; use static allocation for FreeRTOS; no malloc/free in RT paths.
- Logging: strictly off in ISR/RT threads; use lock-free ring buffer to dedicated logger task.
- Release builds: no debug traces in RT code paths.

4) Architectural Direction
- STM32H7 dual-core architecture (CM7/CM4):
  - CM7: Main processing, networking (LWIP), file system (FATFS), CIS processing
  - CM4: GUI, display management, user interface
  - Inter-core communication via shared memory and HSEM
- Module responsibility separation:
  - Core: Hardware abstraction, system initialization
  - Application: High-level application logic
  - Peripheral: Hardware-specific drivers and interfaces
  - Common: Shared utilities and data structures
- Memory regions: respect STM32H7 memory mapping (DTCM, ITCM, AXI SRAM, D1/D2/D3 domains)

5) Style, Quality, and Conventions
- Formatting: clang-format required (LLVM base + Allman braces).
- Analysis: clang-tidy (C++) and cppcheck (C/C++) recommended before merge (at least on changed code).
- Conventions:
  - Include guards or #pragma once; include order: standard → HAL → FreeRTOS → LWIP → internal.
  - Const-correctness and explicit ownership in comments (in English).
  - Clean warnings: -Wall -Wextra. -Werror may be enabled in Debug.
- i18n enforcement in code: no French accents or vocabulary in .c/.h/.cpp/.hpp.
- STM32 HAL usage: prefer HAL over direct register access unless performance critical.

6) Build and Profiles
- Primary targets: STM32H745IIK6 dual-core microcontroller.
- Profiles:
  - Debug: -O0 -g; full debugging symbols; FreeRTOS debug enabled.
  - Release: -O2 -DNDEBUG; RT strict; optimized for embedded performance.
  - Memory regions: FLASH (2MB), RAM (1MB), respect linker script constraints.
- Build systems:
  - STM32CubeIDE generated Makefiles
  - Support for both CM4 and CM7 cores
  - Conditional compilation for different hardware configurations

7) Git, Branching, Versioning
- Branches: protected main, integration dev, feature/*, fix/*.
- Versioning: SemVer with git tags and maintained CHANGELOG.
- Commits: Conventional Commits in English; keep commits atomic.
- STM32CubeIDE files: .ioc files should be committed, generated code should be reviewed.

8) STM32H7 Deployment and Testing
- Method: ST-Link programmer, SWD interface.
- Debugging: STM32CubeIDE debugger, OpenOCD, or similar.
- Verification: Real-time performance monitoring, memory usage analysis, power consumption.
- Hardware-in-the-loop testing when possible.

9) Execution Guardrails for Automation
- Do not start long-running embedded sessions without explicit approval.
- Allowed by default (non-intrusive):
  - Build commands for CM4/CM7 cores
  - Static analysis tools
  - Code formatting and linting
- Intrusive actions (require explicit approval): 
  - Flashing firmware to hardware
  - Modifying .ioc files (STM32CubeMX configuration)
  - Changing memory layout or linker scripts

10) Living Documentation
- Review these rules regularly (e.g., quarterly).
- Update rules when adopting new STM32 HAL versions, FreeRTOS updates, or new peripherals.
- Document hardware-specific constraints and timing requirements.
