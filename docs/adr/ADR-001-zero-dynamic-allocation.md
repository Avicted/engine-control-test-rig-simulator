# ADR-001: Zero Dynamic Allocation

## Status

Accepted

## Context

The engine control test rig simulator must model the deterministic behavior expected of safety-critical embedded control systems. In production IEC 61508 / DO-178C environments, dynamic memory allocation (`malloc`, `calloc`, `realloc`, `free`) is prohibited or heavily restricted because:

1. **Heap fragmentation** causes non-deterministic allocation failures after extended runtime
2. **Allocation failure paths** are difficult to test exhaustively
3. **Worst-case execution time (WCET)** analysis cannot bound allocator latency
4. **Memory leak detection** requires runtime instrumentation that may not be available on target

## Decision

The simulator uses **zero dynamic memory allocation**. All data structures use:

- Stack-allocated local variables
- File-scope (`static`) global state with fixed sizes
- Compile-time constant array bounds (e.g., `HAL_MAX_RX_FRAMES = 32`, `ENGINE_FAULT_COUNTER_COUNT = 3`)
- `_Static_assert` to verify sizes at compile time

No call to `malloc`, `calloc`, `realloc`, or `free` appears anywhere in the codebase.

## Consequences

**Positive:**
- Deterministic memory footprint - stack usage is analyzable by static tools
- No heap fragmentation, no memory leaks
- Simplified certification argument for safety-critical code review
- All buffer sizes are known at compile time, enabling static overflow analysis

**Negative:**
- Maximum capacities must be chosen at design time (e.g., 32-frame FIFO)
- Cannot dynamically scale to arbitrary scenario sizes (bounded to compile-time constants)
- Some patterns (dynamic collections, string building) require more verbose code

**Trade-off accepted:** The bounded-capacity design is appropriate for an engine control simulator where the operational envelope is well-defined.

## Date

2026-02-25
