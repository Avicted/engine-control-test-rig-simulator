# Engine Control Validation & HIL Test Rig Simulator

> **Status**: Feature-complete · 161 unit tests · 80%+ coverage gate · MISRA C:2012 analysis · Static analysis clean

A deterministic C11 engine-control validation simulator designed with
safety-critical principles and CI-driven hardware-in-the-loop (HIL)
workflows in mind.

> [!NOTE]
> Portfolio project demonstrating safety-critical embedded simulation design principles.
> Not intended for real-world control applications.

### Why This Project?

Industrial engine control systems require rigorous verification before deployment.
This simulator demonstrates the software engineering practices used in that domain -
layered architecture, formal traceability, static analysis, and safety analysis -
applied to a simplified but realistic engine control model.



## Prerequisites

**Required to build and run:**

| Tool | Purpose |
|------|---------|
| `clang` | C11 compiler (default in Makefile) |
| `make` | Build system |
| `git` | Embeds commit hash in build metadata |

**Required for the full quality gate (`make ci-check`):**

| Tool | Purpose |
|------|---------|
| `cppcheck` | Static analysis and MISRA C:2012 checking |
| `clang-tidy` | Clang-based static analysis |
| `llvm-cov` | Code coverage reporting (`llvm-cov gcov`) |
| `lcov` | HTML coverage report generation (`make coverage-html`) |
| `python3` | JSON schema contract validation |

**Optional:**

| Tool | Purpose |
|------|---------|
| `raylib` | Visualization dashboard build (`make visualizer`) |
| `doxygen` | API documentation generation (`make docs`) |

On Debian/Ubuntu:

```bash
sudo apt install clang llvm make git cppcheck clang-tidy python3 lcov
```

On macOS (Homebrew):

```bash
brew install llvm make git cppcheck python3 lcov
```



## Quick Start

```bash
make                  # Build the simulator
make test-unit        # Run 161 unit tests
make ci-check         # Full quality gate (build + analysis + tests + coverage)
```

Run a scripted scenario:

```
$ ./build/testrig --script scenarios/normal_operation.txt
[INFO] Scripted scenario evaluated: OK
Script 'scenarios/normal_operation.txt' result: OK
```

Run all validation scenarios with requirement traceability:

```
$ ./build/testrig --run-all
REQ-ENG-003 | normal_operation         | expected=OK       | actual=OK       | PASS
REQ-ENG-001 | overheat_ge_persistence  | expected=SHUTDOWN | actual=SHUTDOWN | PASS
REQ-ENG-002 | oil_low_ge_persistence   | expected=SHUTDOWN | actual=SHUTDOWN | PASS
REQ-ENG-003 | combined_warning_persist | expected=WARNING  | actual=WARNING  | PASS
...
Summary: 10/10 tests passed
```

JSON machine-readable output for CI gating:

```
$ ./build/testrig --script scenarios/normal_operation.txt --json
{
  "schema_version": "1.0.0",
  "software_version": "1.2.1",
  "build_commit": "b16d190",
  "scenarios": [
    {
      "scenario": "scripted_scenario",
      "requirement_id": "REQ-ENG-SCRIPT",
      "ticks": [
        {"tick": 1, "rpm": 2200.00, "temp": 76.00, "oil": 3.20, "run": 1,
         "result": "OK", "control": 31.25, "engine_mode": "STARTING"},
        ...
      ],
      "expected": "OK",
      "actual": "OK",
      "pass": true
    }
  ],
  "summary": {"passed": 1, "total": 1}
}
```



## Architectural Overview

```
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
```

Strict separation ensures simulation determinism is never affected by
visualization logic.

Core data path:

```
  Scenario Input → BusFrame Queue → HAL (Decode + Validate) → Engine → Control → HAL
```



## Layered Source Structure

```
src/
├── app/          CLI and execution orchestration
├── domain/       Engine state machine + control safety logic
├── platform/     HAL boundary and I/O adaptation
├── scenario/     Scenario catalog, profiles, parser, reporting
└── reporting/    Output formatting, logger, report metadata
include/          Public API headers and contracts
```

### Dependency Direction Rules

```
app → domain, platform, scenario, reporting, include
scenario → domain, platform, reporting, include
reporting → domain, platform, include
platform → domain, include
domain → include only
```

These rules are enforced in CI via `make analyze-layering` using `tools/check_layering.sh`.



## Safety-Oriented Design Principles

**Safety & Determinism**
- No dynamic memory allocation (zero `malloc`/`free`)
- No recursion or `goto`
- Bounded loops only; fixed-size buffers and registries
- Fail-safe defaults on all error paths
- No system time, randomness, threads, or external I/O influence

**Testability & Traceability**
- Every requirement traceable to unit tests and integration scenarios
- Explicit pointer validation and status-code-based error handling
- Strict compiler diagnostics (`-Wall -Wextra -Werror -pedantic`)

**Analyzability**
- MISRA C:2012 analysis with 14 formally documented deviations
- cppcheck + clang-tidy + AddressSanitizer + UBSan quality gates
- Layered architecture with enforced dependency direction



## Determinism Guarantee

For identical input scenario files:

- Sensor values are processed sequentially
- State transitions are deterministic
- JSON output is bit-exact reproducible
- No system time, randomness, threads, or external I/O influence results

This enables reproducible CI regression validation and controlled
HIL-style replay.



## System Model

### Inputs

| Parameter | Type | Description |
|-----------|------|-------------|
| RPM | `float` | Engine rotational speed |
| Temperature | `float` | Exhaust temperature (°C) |
| Oil Pressure | `float` | Lubrication pressure (bar) |
| Running | `int` | Engine running state (0/1) |

### Engine Modes

`INIT` → `STARTING` → `RUNNING` → `WARNING` → `SHUTDOWN`

### Temporal Fault Validation

- Temperature persistence threshold → SHUTDOWN
- Oil pressure persistence threshold → SHUTDOWN
- Combined RPM + temperature warning persistence → WARNING

Fault escalation requires deterministic multi-tick persistence,
mirroring industrial safety logic.



## Scripted Scenario Support

Deterministic scenarios can be defined without modifying source code:

```text
TICK 1 RPM 2200 TEMP 76 OIL 3.2 RUN 1
TICK 2 RPM 2600 TEMP 80 OIL 3.1 RUN 1
TICK 3 RPM 3000 TEMP 83 OIL 3.0 RUN 1
TICK 4 RPM 3200 TEMP 84.5 OIL 2.9 RUN 1
TICK 5 FRAME CORRUPT
```

Run:

```bash
./build/testrig --script scenario.txt --json
```

Corrupt frame directives are accepted and routed through HAL checksum validation.
See `scenarios/README.md` for the full catalog of included scenarios.



## JSON Machine Output

Per-scenario JSON includes:

- Top-level contract metadata (`schema_version`, `software_version`, `build_commit`)
- Tick-level sensor inputs, control output, and engine mode
- Requirement traceability ID (`requirement_id`)
- Scenario expected vs actual outcome with pass/fail status

Formal schema: `schema/engine_test_rig.schema.json`

Expected CLI behavior:

- Exit code `0` for a passing scenario
- JSON envelope includes `schema_version`, `software_version`, `build_commit`, `scenarios`, and `summary`
- On failure paths, JSON includes `error` with `code`, `module`, `function`, `tick`, `severity`, and `recoverability`



## Raylib Visualization (Read-Only)

![Raylib Visualizer Screenshot](visualization/visualizer_example.png)

The visualization program reads JSON output only - it cannot call simulator internals or modify simulation state.

Features:

- Animated dashboard playback with timeline graphs
- Threshold overlays and scrubbable tick slider
- Multi-scenario switching with cumulative statistics



## Module API Overview

Primary public interfaces are intentionally narrow and defined under `include/`:

| Header | Key Functions | Purpose |
|--------|--------------|---------|
| `hal.h` | `hal_ingest_sensor_frame()`, `hal_read_sensors()`, `hal_receive_bus()`, `hal_transmit_bus()`, `hal_vote_sensors()`, `hal_watchdog_check()` | Deterministic sensor transport, bus I/O, sensor voting, watchdog, structured diagnostics |
| `control.h` | `evaluate_engine()`, `compute_control_output()`, `control_configure_calibration()` | Persistence-threshold safety rules, actuator demand, calibration |
| `engine.h` | `engine_start()`, `engine_update()`, `engine_transition_mode()` | State machine transitions, physics update |
| `script_parser.h` | `script_parser_parse_file()` | Scenario script validation and tick/frame emission |
| `config.h` | `config_load_calibration_file()`, `config_load_physics_file()` | JSON calibration and physics configuration loading |
| `test_runner.h` | `run_all_tests()`, `run_scripted_scenario_with_json()` | Scenario orchestration and JSON/console entry points |



## Hardware Abstraction Layer

HAL interfaces are defined in `include/hal.h` and implemented in `src/platform/hal.c`.

- `hal_init()` / `hal_shutdown()` - HAL lifecycle management
- `hal_ingest_sensor_frame()` - deterministic transport frame ingestion
- `hal_read_sensors()` - frame payload decode with timeout detection
- `hal_apply_sensors()` - validated sensor-to-engine state boundary
- `hal_receive_bus()` / `hal_transmit_bus()` - deterministic bus interfaces
- `hal_write_actuators()` - control egress boundary
- `hal_submit_redundant_temp()` / `hal_vote_sensors()` - dual-channel sensor voting

`BusFrame` is ABI-hardened: `_Static_assert(sizeof(BusFrame) == 13, "Unexpected frame size");`



## Requirement Traceability

Requirement IDs are defined in `src/scenario/requirements.h` and attached to test
registry entries.

Console output includes requirement linkage per test:

```
REQ-ENG-002 | oil_low_ge_persistence | expected=SHUTDOWN | actual=SHUTDOWN | PASS
```

JSON output includes `"requirement_id": "REQ-ENG-002"` per scenario.

Full requirement-to-test mapping: `docs/requirements_traceability_matrix.md`



## Error Handling Model

`include/status.h` defines the unified status model:

| Status | Severity | Recoverability |
|--------|----------|----------------|
| `STATUS_OK` | `SEVERITY_INFO` | - |
| `STATUS_INVALID_ARGUMENT` | `SEVERITY_ERROR` | `RECOVERABLE` |
| `STATUS_PARSE_ERROR` | `SEVERITY_ERROR` | `RECOVERABLE` |
| `STATUS_IO_ERROR` | `SEVERITY_ERROR` | `NON_RECOVERABLE` |
| `STATUS_TIMEOUT` | `SEVERITY_FATAL` | `RECOVERABLE` |
| `STATUS_BUFFER_OVERFLOW` | `SEVERITY_WARNING` | `RECOVERABLE` |
| `STATUS_INTERNAL_ERROR` | `SEVERITY_FATAL` | `NON_RECOVERABLE` |

All paths use explicit status returns with no silent fallthrough.
Strict mode is available via `--strict`.



## Calibration Configuration

Threshold calibration can be loaded at startup:

```bash
./build/testrig --run-all --config calibration.json
```

Supported keys: `temperature_limit`, `oil_pressure_limit`, `persistence_ticks`, `combined_warning_persistence_ticks` (optional)

Schema: `schema/calibration.schema.json`. Configuration is parsed once at startup
and cannot be mutated during runtime. If `--config` is omitted, deterministic defaults are used.



## Quality Gates

`make ci-check` is the baseline quality gate. It fails if any of the following fail:

| Gate | Command | Threshold |
|------|---------|-----------|
| Compiler | `make all` | Zero warnings (`-Werror`) |
| cppcheck | `make analyze-cppcheck` | Zero findings |
| clang-tidy | `make analyze-clang-tidy` | Zero findings |
| Layering | `make analyze-layering` | No dependency violations |
| Sanitizers | `make analyze-sanitizers` | No ASan/UBSan findings |
| Unit tests | `make test-unit` | 161/161 pass |
| Integration | `make test-all` | All scenarios pass |
| JSON contract | `make validate-json` | Schema-valid output |
| Coverage | `make coverage` | ≥ 80% source-only lines |
| Coverage HTML | `make coverage-html` | Browsable HTML report in `coverage/html/` |



## CLI Reference

```
Usage:
  Global options: [--config calibration.json] [--log-level DEBUG|INFO|WARN|ERROR]

  ./build/testrig --run-all [--show-sim] [--show-control] [--show-state] [--color] [--json] [--strict]
  ./build/testrig --scenario <name> [--show-sim] [--show-control] [--show-state] [--color] [--json] [--strict]
  ./build/testrig --script <path> [--show-sim] [--show-control] [--show-state] [--color] [--json] [--strict]
```

| Flag | Description |
|------|-------------|
| `--run-all` | Execute all built-in validation scenarios |
| `--scenario <name>` | Run a single named scenario |
| `--script <path>` | Run a scripted scenario file |
| `--json` | Emit JSON machine-readable output |
| `--config <path>` | Load calibration thresholds from JSON |
| `--log-level <level>` | Set log verbosity (`DEBUG`, `INFO`, `WARN`, `ERROR`) |
| `--color` | Enable ANSI color output (disabled by default for CI) |
| `--strict` | Strict validation mode |
| `--show-sim` | Display per-tick simulation values during execution |
| `--show-control` | Display per-tick control output during execution |
| `--show-state` | Display per-tick engine state transitions during execution |



## Documentation

| Document | Description |
|----------|-------------|
| `docs/safety_case.md` | Structured safety argument with hazard identification and safety functions |
| `docs/fmea.md` | 15-item Failure Mode and Effects Analysis with RPN ratings |
| `docs/requirements_traceability_matrix.md` | Full requirement → scenario → unit test mapping |
| `docs/misra_deviations.md` | 14 formally documented MISRA C:2012 deviations with rationale |
| `docs/review_checklist.md` | Repeatable technical review procedure |
| `docs/static_analysis_baseline_policy.md` | Zero-warning baseline policy |
| `docs/adr/` | Architecture Decision Records (ADR-001 through ADR-004) |
| `CHANGELOG.md` | Version history following Keep a Changelog conventions |



## Deliberate Scope Boundaries

The following are natural extensions intentionally left out of scope:

- **Hardware-in-the-loop interface** - CAN/Modbus bridge to a real ECU
- **Fault-type classification reporting** - categorized fault event log export
- **CSV export** - tabular format for lab analysis tooling



## License

MIT License.
