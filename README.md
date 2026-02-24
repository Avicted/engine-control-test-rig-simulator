# Engine Control Test Rig Simulator

A deterministic C11 engine-control simulator implemented with safety-oriented rules inspired by NASA/JPL Power of Ten practices.

## Project Description

This project models a simplified engine state and validates safety control outcomes under deterministic scenarios. It is structured for predictable, analyzable behavior suitable for CI pipelines and embedded-adjacent validation workflows.

Safety-focused characteristics:
- Deterministic execution (no randomness, no time-based behavior)
- No dynamic memory allocation
- Explicit pointer validation and status-code returns
- Bounded loops and fixed-size test arrays
- Strict compiler diagnostics with warnings treated as errors
- Structured, bounded logging output

## Architecture

- `src/engine.c`, `src/engine.h`  
  Engine state model and deterministic update loop.
- `src/sensors.c`, `src/sensors.h`  
  Deterministic scenario injection (normal ramp-up, overheat, oil pressure failure).
- `src/control.c`, `src/control.h`  
  Deterministic safety threshold evaluation (`ENGINE_OK`, `ENGINE_WARNING`, `ENGINE_SHUTDOWN`, `ENGINE_ERROR`).
- `src/test_runner.c`, `src/test_runner.h`  
  Fixed-size automated validation framework (`MAX_TESTS=6`) with deterministic PASS/FAIL reporting.
- `src/logger.c`, `src/logger.h`  
  Bounded logging implementation using `snprintf` with return-value checks.
- `src/main.c`  
  CLI argument handling.

## Build

From the project directory:

```bash
make
```

The `Makefile` compiles in safety mode:

`-std=c11 -Wall -Wextra -Werror -pedantic -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2`

Clean artifacts:

```bash
make clean
```

## Run

Run full automated validation suite:

```bash
./testrig --run-all
./testrig --run-all --show-sim
./testrig --run-all --show-control
./testrig --run-all --color
./testrig --run-all --show-sim --show-control --color
```

Run a single scenario:

```bash
./testrig --scenario overheat
./testrig --scenario normal
./testrig --scenario pressure_failure
./testrig --scenario normal --show-sim
./testrig --scenario normal --show-control
./testrig --scenario overheat --color
./testrig --scenario normal --show-sim --show-control --color
```

## Example Output

`--run-all`:

```text
[INFO] Running automated validation tests
PASS: normal_operation (expected=OK, actual=OK)
PASS: overheat_test (expected=SHUTDOWN, actual=SHUTDOWN)
PASS: pressure_failure (expected=SHUTDOWN, actual=SHUTDOWN)
PASS: warning_high_rpm_temp (expected=WARNING, actual=WARNING)
PASS: temp_threshold_ok (expected=OK, actual=OK)
PASS: oil_threshold_ok (expected=OK, actual=OK)
Summary: 6/6 tests passed
[INFO] All tests passed
```

`--show-sim` prints deterministic engine-state snapshots used in evaluation:

```text
SIM: normal_operation rpm=120.00 temp=25.70 oil=3.60 running=1
```

`--show-control` prints the deterministic control output computed from engine state:

```text
CTRL | output= 18.57%
```

`--color` optionally colorizes PASS/FAIL, result statuses, and `[INFO]/[WARN]/[ERROR]` log levels for readability in interactive terminals. It is disabled by default for CI-safe plain output.

Single scenario (`--scenario overheat`):

```text
[ERROR] Scenario evaluated: SHUTDOWN
Scenario 'overheat' result: SHUTDOWN
```

## CI Integration

The binary is CI-ready:
- `./testrig --run-all` returns exit code `0` when all tests pass
- `./testrig --run-all` returns exit code `1` if any test fails

This supports straightforward CI steps:

```bash
make
./testrig --run-all
```

## Safety Principles Mapping

- No dynamic memory (`malloc/calloc/realloc/free` are not used)
- No recursion and no `goto`
- Bounded execution paths (fixed-size loops, fixed test count)
- Input validation in module APIs (pointer checks and explicit errors)
- No global mutable state
- Deterministic, reproducible scenario outcomes
- Static-analysis-friendly control flow and explicit branches

## HIL Simulation Rationale

In a real HIL setup, a controller is validated against reproducible plant/sensor conditions. This simulator mirrors that model by:
- Driving deterministic state transitions
- Injecting repeatable fault scenarios
- Enforcing safety shutdown logic
- Producing bounded, structured validation output


## License
MIT License. See [LICENSE](LICENSE) for details.