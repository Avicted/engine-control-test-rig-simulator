# ADR-002: Deterministic Tick-Based Simulation

## Status

Accepted

## Context

Real engine control systems operate in continuous time with hardware interrupts, RTOS task scheduling, and sub-millisecond timing constraints. Modeling real-time behavior requires either:

1. A real-time simulation framework (e.g., FreeRTOS + timing model)
2. A deterministic discrete-event model where time advances in logical ticks

Option 1 introduces OS dependencies, non-deterministic scheduling, and makes tests platform-dependent. Option 2 sacrifices timing fidelity but guarantees bit-exact reproducibility.

## Decision

The simulator uses a **deterministic tick-based execution model**:

- Time is represented as a monotonically increasing `uint32_t tick` counter
- Each tick processes exactly one sensor frame and one evaluation cycle
- No system clock, no `time()`, no `clock_gettime()`, no `sleep()`
- No threads, no mutexes, no atomic operations
- Scenarios specify sensor values at exact tick numbers

## Consequences

**Positive:**
- **Bit-exact reproducibility**: same scenario → same output, every run, every platform
- **Trivial CI gating**: expected outputs can be compared deterministically
- **No test flakiness**: no timing races, no thread scheduling variations
- **Simplified analysis**: control flow is fully sequential, WCET is trivially bounded
- **Portable**: runs identically on x86, ARM, CI containers

**Negative:**
- Does not exercise real-time concerns (interrupt latency, priority inversion, watchdog reset timing)
- Tick duration has no physical time mapping - cannot validate timing requirements
- Cannot model jitter, clock drift, or asynchronous sensor arrival

**Trade-off accepted:** The tick model validates *logical correctness* of fault detection and response. Real-time validation would require a separate HIL (Hardware-in-the-Loop) test bench, which is out of scope for a software-only simulator.

## Date

2026-02-25
