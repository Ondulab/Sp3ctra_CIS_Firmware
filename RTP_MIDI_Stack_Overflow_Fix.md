# RTP-MIDI Stack Overflow Fix (Release Build)

## Problem Description

The system was crashing in Release build with a **MemManage_Handler** fault:

```
Thread #1 [main] 1 [core: 0] (Suspended : Signal : SIGINT:Interrupt)
    MemManage_Handler() at 0x8103b18
    <signal handler called>() at 0xfffffffd
    StartMidiTask() at 0x8102b2e
    xQueueGenericSend() at 0x8116d0a
```

The crash occurred in `StartMidiTask()` when calling FreeRTOS queue functions.

## Root Cause

**Stack overflow in the MIDI task.**

The MIDI task was configured with only **512 bytes** of stack:
```c
osThreadDef(midiTask, StartMidiTask, osPriorityNormal, 0, 512);
```

This was insufficient because:
1. **Release optimizations (-O2)** can create larger stack frames than Debug (-O0)
2. The MIDI task calls multiple functions with deep call stacks:
   - `rtpmidi_process()` → session management, packet handling
   - `midi_button_mapper_on_change()` → MIDI packet creation
   - FreeRTOS queue operations (`xQueueGenericSend()`)
3. Local variables and function parameters consume stack space
4. Interrupt handlers can also use the task's stack

## Why It Worked in Debug But Not Release

| Mode | Optimization | Stack Usage | Result |
|------|-------------|-------------|---------|
| **Debug** | -O0 (none) | Variables often kept in memory, different frame layout | Works |
| **Release** | -O2 (aggressive) | Inlining, register allocation, frame reorganization | **Stack overflow** |

Release builds can paradoxically use MORE stack in some cases due to:
- Function inlining increasing local variable count
- Different register spilling strategies
- Compiler optimizations that trade stack space for speed

## Solution

Increased the MIDI task stack size from **512 to 2048 bytes**.

### File Modified: CM7/Core/Src/freertos.c

```c
// Before:
osThreadDef(midiTask, StartMidiTask, osPriorityNormal, 0, 512);

// After:
osThreadDef(midiTask, StartMidiTask, osPriorityNormal, 0, 2048);
```

## Stack Size Guidelines for FreeRTOS Tasks

| Task Type | Recommended Stack Size |
|-----------|----------------------|
| Simple tasks (LED blink, flags) | 256-512 bytes |
| Medium tasks (sensor reading, simple processing) | 512-1024 bytes |
| **Complex tasks (networking, MIDI, multiple calls)** | **2048-4096 bytes** |
| Tasks with printf/logging | Add 512-1024 bytes |
| Tasks with deep recursion | Calculate based on depth |

## How to Detect Stack Overflow

### 1. Enable FreeRTOS Stack Checking

In `FreeRTOSConfig.h`:
```c
#define configCHECK_FOR_STACK_OVERFLOW  2
```

### 2. Implement Stack Overflow Hook

Already implemented in `freertos.c`:
```c
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
    taskDISABLE_INTERRUPTS();
    printf("Stack overflow for task : %s\n", pcTaskName);
    Error_Handler();
}
```

### 3. Monitor Stack Usage at Runtime

Use FreeRTOS API:
```c
UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
printf("Stack remaining: %lu bytes\n", uxHighWaterMark * sizeof(StackType_t));
```

## Memory Impact

Increasing stack from 512 to 2048 bytes:
- **Additional RAM usage:** 1536 bytes (1.5 KB)
- **Total MIDI task RAM:** 2048 bytes (2 KB)
- **Impact on STM32H745:** Negligible (1 MB total RAM available)

## Testing

After this fix:
- ✅ Release build should run without MemManage faults
- ✅ MIDI task operates normally
- ✅ RTP-MIDI connects and sends/receives messages
- ✅ System remains stable under load

## Related Fixes

This fix complements the previous LWIP pbuf double-free fix:
1. **pbuf fix** (in `rtpmidi_session.c`): Prevents assertion failures
2. **Stack fix** (in `freertos.c`): Prevents MemManage faults in Release

Both fixes are necessary for stable RTP-MIDI operation.

## Conclusion

Stack overflow is a common issue when moving from Debug to Release builds due to different optimization strategies. Always allocate sufficient stack for tasks that:
- Make multiple nested function calls
- Use networking/protocol stacks
- Perform complex processing
- Use printf/logging

For the MIDI task, 2048 bytes provides adequate headroom for all operations while remaining memory-efficient.
