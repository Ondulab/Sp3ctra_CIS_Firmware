1) Purpose and Scope
- Purpose: Provide a clear, safe, and repeatable way for Cline to plan and implement changes while respecting real-time (RT) embedded constraints and STM32H7 dual-core architecture.
- Scope: Applies to all Cline-led work within this repository.
- Separation of concerns:
  - Rules = how code must be written (see custom_instructions.md).
  - Workflow = how Cline must execute work (this file).

2) Operating Principles
- Always start in Plan Mode; only implement in Act Mode after explicit approval.
- Minimize RT risk: no blocking operations in ISR/DMA callbacks, respect FreeRTOS task priorities.
- Prefer small, atomic changes with clear acceptance criteria.
- Never modify files outside this repository.
- Use idiomatic and measurable phrasing in plans (avoid vague directives like "optimize where possible").

3) Plan Mode (analysis and planning)
1. Context gathering
   - Read the relevant files (README, .ioc configuration, specific module files).
   - Identify any RT hot path impact and dual-core communication constraints.
2. Questions and assumptions
   - Confirm target cores (CM7/CM4), memory domains (D1/D2/D3), and RT budgets.
   - Clarify testability needs (must work without hardware?).
3. Proposed plan
   - List exact files to add/modify/remove.
   - Outline approach, risks, and rollback strategy.
   - Consider inter-core communication impact (HSEM, shared memory).
4. Acceptance criteria (measurable)
   - Build succeeds for both CM4 and CM7 cores.
   - Strict RT rules respected (no alloc/lock/log in ISR/DMA callbacks).
   - clang-format applied; no French in code; compile warnings clean where feasible.
   - Memory usage within STM32H7 constraints (FLASH: 2MB, RAM: 1MB).
5. Execution guardrails
   - Flag any intrusive action requiring approval (hardware flashing, .ioc modifications, linker changes).
   - Provide the command(s) to be run and their expected, short-lived nature.

4) Act Mode (implementation)
1. Editing rules
   - Prefer replace_in_file for small, localized changes.
   - Use write_to_file for new files or major rewrites.
   - Keep changes minimal and incremental to limit RT risk.
2. Execution guardrails
   - Allowed by default (short, non-intrusive):
     - Build commands: make -C CM7/Debug, make -C CM4/Debug
     - Static analysis tools (clang-tidy, cppcheck)
     - Code formatting (clang-format)
   - Requires explicit approval:
     - Hardware flashing or debugging sessions
     - Modifying .ioc files (STM32CubeMX configuration)
     - Changing linker scripts or memory layout
     - System configuration changes
   - Never edit outside this repository.
3. Local quality checks
   - Apply clang-format (LLVM + Allman).
   - Enforce English-only in code (no French words or accents).
   - Build both cores by default.
   - Verify memory usage fits STM32H7 constraints.
4. Output and traceability
   - Use Conventional Commits (English).
   - Keep commits atomic and scoped.
   - Document RT-related risks and provide rollback steps in PR description.

5) Checklists (copy into plan/PR as needed)
A. "PR ready" checklist
- [ ] Build succeeds for both CM4 and CM7 cores.
- [ ] No French text in code (.c/.h/.cpp/.hpp).
- [ ] clang-format (LLVM + Allman) applied to changed files.
- [ ] No allocation/lock/log inside ISR, DMA callbacks, or RT hot paths.
- [ ] Memory usage within STM32H7 constraints (FLASH/RAM).
- [ ] Inter-core communication properly synchronized (HSEM).
- [ ] Conventional Commit(s) in English; atomic changes.

B. "RT change" checklist (when touching RT paths)
- [ ] ISR execution time target respected (< 10% of interrupt period).
- [ ] No blocking I/O or mutex in ISR/DMA callbacks.
- [ ] Preallocated buffers; no runtime (re)allocations in RT paths.
- [ ] FreeRTOS task priorities and deadlines respected.
- [ ] Logging routed via lock-free queue to non-RT logger task (if needed).

C. "Dual-core change" checklist (when affecting CM4/CM7 communication)
- [ ] Shared memory access properly synchronized.
- [ ] HSEM usage follows STM32H7 guidelines.
- [ ] Memory coherency maintained between cores.
- [ ] Boot sequence and core startup order preserved.

6) Playbooks
1. Add a new Peripheral driver (hardware-specific, testable without full system)
- Plan: define header and C file in Peripheral/ with English comments; zero blocking calls in ISR; define ownership and memory usage.
- Act: write_to_file for new files; update build system if needed.
- Validate: format, i18n, build both cores, verify memory constraints.

2. Patch an RT embedded path (ISR, DMA callback, high-frequency task)
- Plan: show the exact section impacted; explain how you avoid alloc/lock/log; define timing budget.
- Act: replace_in_file with minimal changes; no blocking I/O added.
- Validate: build Release; manual verification of RT constraints.

3. Add a new FreeRTOS task
- Plan: document task priority, stack size, and interaction with other tasks; confirm no RT impact.
- Act: patch task creation and relevant modules; ensure English-only logs/messages.
- Validate: build; verify task priorities and memory usage.

4. Update networking/LWIP configuration
- Plan: confine changes to LWIP/ and related config; avoid RT path impact on CM7.
- Act: targeted patches; English logs outside RT only.
- Validate: build; avoid long network sessions without approval.

5. Modify dual-core communication
- Plan: document shared memory layout and synchronization mechanism; verify HSEM usage.
- Act: update both CM4 and CM7 code; maintain memory coherency.
- Validate: build both cores; test inter-core communication if possible.

7) Quality Gates and Acceptance
- Minimal local gate before merge:
  - Both CM4 and CM7 builds OK
  - clang-format applied
  - Zero French in code
  - Manual review focused on RT and dual-core constraints
- Optional (recommended when available):
  - clang-tidy/cppcheck on changed C/C++ code
  - Memory usage analysis
  - Static timing analysis for RT paths

8) Safety Boundaries for Automation
- Never start hardware flashing or debugging without explicit approval.
- Always call out intrusive operations for approval first.
- Do not modify global system configuration, .ioc files, or linker scripts without approval.
- Respect STM32H7 memory domains and dual-core architecture.

9) Maintenance and Evolution
- Treat this workflow as living documentation.
- Review quarterly or when adopting new STM32 HAL versions or FreeRTOS updates.
- Keep MIGRATION.md up to date for reorganizations and terminology changes.

10) Quick Reference (Commands)
- Typical CM7 build: make -C CM7/Debug
- Typical CM4 build: make -C CM4/Debug
- Both cores build: make -C CM7/Debug && make -C CM4/Debug
- Clean build: make -C CM7/Debug clean && make -C CM4/Debug clean
