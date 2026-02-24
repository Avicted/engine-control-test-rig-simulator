# Engine Control Validation & HIL Test Rig Simulator

A deterministic C11 engine-control validation simulator designed with
safety-critical principles and CI-driven hardware-in-the-loop (HIL)
workflows in mind.

This project models a simplified engine state, injects deterministic
fault conditions, validates control-system safety responses, and
supports decoupled visualization via a read-only Raylib dashboard.

------------------------------------------------------------------------

## Architectural Overview

                 +---------------------------+
                 |   testrig (CLI core)      |
                 |---------------------------|
                 | Deterministic simulation  |
                 | Safety validation logic   |
                 | JSON output               |
                 +-------------+-------------+
                               |
                               | JSON (read-only)
                               v
                 +---------------------------+
                 |   visualizer (Raylib)     |
                 |---------------------------|
                 | Read-only JSON playback   |
                 | Animated dashboard        |
                 +---------------------------+

Strict separation ensures simulation determinism is never affected by
visualization logic.

Current core data path:

           +------------------+
           |  Scenario Input  |
           +------------------+
                  |
                  v
              +---------+
              |   HAL   |
              +---------+
              | Sensors |
              | Actuators
                  |
                  v
              +---------+
              | Engine  |
              +---------+
                  |
                  v
              +---------+
              | Control |
              +---------+
                  |
                  v
              +---------+
              |  HAL    |
              +---------+

Visualizer remains read-only and separate from simulation logic.

------------------------------------------------------------------------

## Determinism Guarantee

For identical input scenario files:

-   Sensor values are processed sequentially
-   State transitions are deterministic
-   JSON output is reproducible
-   No system time, randomness, threads, or external IO influence
    results

This enables reproducible CI regression validation and controlled
HIL-style replay.

------------------------------------------------------------------------

## Safety-Oriented Design Principles

-   No dynamic memory allocation
-   No recursion or `goto`
-   Bounded loops only
-   Fixed-size buffers and registries
-   Explicit pointer validation
-   Status-code-based error handling
-   Strict compiler diagnostics (`-Werror`)
-   No hidden global mutable state
-   CI-safe output mode (color disabled by default)

Design prioritizes analyzability and predictability over minimal code
size.

------------------------------------------------------------------------

## System Model

### Inputs

-   RPM
-   Temperature
-   Oil Pressure
-   Engine Running State

### Engine Modes

-   INIT
-   STARTING
-   RUNNING
-   WARNING
-   SHUTDOWN

### Control Results

-   ENGINE_OK
-   ENGINE_WARNING
-   ENGINE_SHUTDOWN
-   ENGINE_ERROR

### Temporal Fault Validation

-   Temperature persistence threshold
-   Oil pressure persistence threshold
-   Combined RPM + temperature warning persistence

Fault escalation requires deterministic multi-tick persistence,
mirroring industrial safety logic.

------------------------------------------------------------------------

## Scripted Scenario Support

Deterministic scenarios can be defined without modifying source code:

TICK 1 RPM 2200 TEMP 76 OIL 3.2 RUN 1 TICK 2 RPM 2600 TEMP 80 OIL 3.1
RUN 1 TICK 3 RPM 3000 TEMP 83 OIL 3.0 RUN 1 TICK 4 RPM 3200 TEMP 84.5
OIL 2.9 RUN 1

Run:

./build/testrig --script scenario.txt --json

Supports CI-driven regression and HIL-style replay workflows.

------------------------------------------------------------------------

## JSON Machine Output

Per-scenario JSON includes:

-   Tick-level sensor inputs
-   Control output
-   Engine mode
-   Requirement traceability ID (`requirement_id`)
-   Evaluation result
-   Scenario expected vs actual outcome
-   Pass/fail status

Designed for CI gating and automated validation parsing.

------------------------------------------------------------------------

## Raylib Visualization (Read-Only)

![Raylib Visualizer Screenshot](visualization/visualizer_example.png)

The visualization program:

-   Reads JSON only
-   Does not call simulator internals
-   Cannot modify simulation state
-   Provides animated dashboard playback

Features:

-   Pixel monospaced font
-   Responsive layout
-   Threshold overlays
-   Timeline graphs
-   Scrubbable tick slider
-   Multi-scenario switching
-   Cumulative warning/shutdown statistics

Raylib chosen for:

-   Lightweight C API
-   Cross-platform support
-   Minimal dependencies
-   Suitable for lab dashboard tooling

------------------------------------------------------------------------

## CI Integration Model

Exit codes:

-   0 → all tests pass
-   1 → any test fails

Example:

make ./build/testrig --run-all

Intended to behave like a validation component gating control-system
changes before hardware deployment.

------------------------------------------------------------------------

## Hardware Abstraction Layer

HAL interfaces are defined in `src/hal.h` and implemented in `src/hal.c`.

-   `hal_init()` / `hal_shutdown()` manage HAL lifecycle
-   `hal_read_sensors()` is the sensor ingress boundary
-   `hal_write_actuators()` is the control egress boundary

The current HAL implementation is deterministic pass-through with strict
pointer validation and explicit `StatusCode` returns.

------------------------------------------------------------------------

## Requirement Traceability

Requirement IDs are defined in `src/requirements.h` and attached to test
registry entries.

Console output includes requirement linkage per test, for example:

`REQ-ENG-002 | oil_low_ge_persistence | expected=SHUTDOWN | actual=SHUTDOWN | PASS`

JSON output includes:

`"requirement_id": "REQ-ENG-002"`

------------------------------------------------------------------------

## Error Handling Model

`src/status.h` defines the unified status model:

-   `STATUS_OK`
-   `STATUS_INVALID_ARGUMENT`
-   `STATUS_PARSE_ERROR`
-   `STATUS_IO_ERROR`
-   `STATUS_BUFFER_OVERFLOW`
-   `STATUS_INTERNAL_ERROR`

Script parsing and logger/HAL paths use explicit status returns with no
silent fallthrough.

Strict mode is available via `--strict`.

------------------------------------------------------------------------

## Static Analysis Compatibility

Make targets:

-   `make analyze-cppcheck`
-   `make analyze-clang-tidy`
-   `make debug` (AddressSanitizer + UndefinedBehaviorSanitizer)

------------------------------------------------------------------------

## Failure Modes

Common hard-fail conditions:

-   Invalid CLI arguments
-   Malformed script lines
-   Non-increasing script tick sequence
-   File open/close errors
-   Output buffer overflow checks in formatted writes

These failures return non-zero exit status for CI safety.

------------------------------------------------------------------------

## Design Tradeoffs

This simulator intentionally:

-   Avoids dynamic allocation for analyzability
-   Uses explicit branching for clarity
-   Separates presentation from validation logic
-   Favors deterministic reproducibility over compact code

The goal is robustness and reasoning transparency.

------------------------------------------------------------------------

## Industrial Relevance

Demonstrates:

-   Control-logic validation mindset
-   Fault injection discipline
-   Deterministic simulation design
-   CI-driven validation workflows
-   Defensive C programming
-   Modular lab-style architecture
-   Decoupled presentation layer

Structured to resemble a minimal automation laboratory validation
component.

------------------------------------------------------------------------

## Future Extensions

-   Configurable thresholds via JSON
-   Fault-type classification reporting
-   CSV export for lab analysis
-   Replay checksum verification
-   Module-level unit testing integration

------------------------------------------------------------------------

## License

MIT License.
