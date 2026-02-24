# Engine Control Test Rig Simulator

A deterministic C11 engine-control validation simulator designed with
safety-critical principles and CI-driven hardware-in-the-loop (HIL)
workflows in mind.

This project models a simplified engine state, injects deterministic
fault conditions, and validates control-system safety responses under
reproducible scenarios. It is structured to resemble an industrial test
rig component used in automated validation environments.


## Project Intent

This simulator is designed to demonstrate:

-   Deterministic control-system validation
-   Reproducible fault injection
-   Safety-oriented C implementation discipline
-   CI-compatible automated testing
-   Hardware-in-the-loop style scenario modeling
-   Static-analysis-friendly code structure

The architecture mirrors how engine control functionality can be
validated in lab environments before deployment to production systems.



## Safety-Oriented Design

The implementation follows safety-driven C practices inspired by
aerospace and embedded system standards.

Key characteristics:

-   Deterministic execution (no randomness, no time dependency)
-   No dynamic memory allocation
-   No recursion and no `goto`
-   Fixed-size test registry (`MAX_TESTS=6`)
-   Bounded loops only
-   Explicit pointer validation and status-code returns
-   Strict compiler diagnostics (`-Werror`)
-   Structured and bounded logging (`snprintf` with checks)
-   No global mutable state
-   CI-safe output mode (color optional, disabled by default)

The goal is predictable, analyzable behavior suitable for automated
validation pipelines.



## System Model

The simulator models:

-   Engine RPM
-   Engine temperature
-   Oil pressure
-   Engine running state
-   Engine mode state machine (`INIT`, `STARTING`, `RUNNING`, `WARNING`, `SHUTDOWN`)

Control outputs:

-   `ENGINE_OK`
-   `ENGINE_WARNING`
-   `ENGINE_SHUTDOWN`
-   `ENGINE_ERROR`

Safety thresholds:

-   Temperature limit enforcement with temporal persistence (`TEMP_PERSISTENCE_TICKS`)
-   Oil pressure minimum enforcement with temporal persistence (`OIL_PRESSURE_PERSISTENCE_TICKS`)
-   Combined RPM + temperature warning behavior with persistence (`RPM_TEMP_WARNING_PERSISTENCE_TICKS`)

This structure mirrors how control logic in real engine control systems
must respond deterministically to sensor inputs and fault conditions.



## Architecture

-   `engine.*`\
    Deterministic engine state model and update behavior.

-   `sensors.*`\
    Repeatable fault injection scenarios:

    -   Normal ramp-up
    -   Overheat condition
    -   Oil pressure failure
    -   Threshold edge cases

-   `control.*`\
    Explicit safety-rule evaluation with bounded branching and no
    implicit fallthrough.

-   `test_runner.*`\
    Fixed-size automated validation harness suitable for CI integration.

-   `logger.*`\
    Structured, bounded logging with buffer protection.

-   `main.c`\
    Strict CLI parsing and argument validation.

Each module is isolated and free of hidden global state, enabling clear
reasoning and static analysis.



## Build

Compile in safety mode:

``` bash
make
```

Compiler flags:

    -std=c11
    -Wall -Wextra -Werror -pedantic
    -O2
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2

Clean:

``` bash
make clean
```



## Run

Run automated validation suite:

``` bash
./build/testrig --run-all
```

Optional debugging flags:

-   `--show-sim` → print deterministic engine-state snapshots
-   `--show-control` → print computed control outputs
-   `--show-state` → print engine mode and fault counters per tick
-   `--color` → interactive terminal readability (disabled by default
    for CI)

Run individual scenario:

``` bash
./build/testrig --scenario overheat
```



## Example Output

Automated validation:

    [INFO] Running automated validation tests
    TEST | normal_operation
    EVAL | expected=OK        actual=OK        => PASS
    TEST | overheat_lt_persistence
    EVAL | expected=OK        actual=OK        => PASS
    TEST | overheat_ge_persistence
    EVAL | expected=SHUTDOWN  actual=SHUTDOWN  => PASS
    TEST | oil_low_ge_persistence
    EVAL | expected=SHUTDOWN  actual=SHUTDOWN  => PASS
    TEST | combined_warning_persistence
    EVAL | expected=WARNING   actual=WARNING   => PASS
    Summary: 5/5 tests passed
    [INFO] All tests passed

Single scenario:

    [ERROR] Scenario evaluated: SHUTDOWN
    Scenario 'overheat' result: SHUTDOWN



## CI Integration Model

The simulator is designed to behave like a lab validation component
integrated into a Continuous Integration pipeline.

-   Exit code `0` → all validation tests pass
-   Exit code `1` → any validation test fails

CI example:

``` bash
make
./build/testrig --run-all
```

This mirrors how automated test rigs can gate changes to control
software before integration into physical hardware environments.



## Hardware-in-the-Loop (HIL) Rationale

In real HIL systems:

-   A controller is validated against deterministic plant simulations
-   Fault conditions are injected in a controlled and repeatable manner
-   Safety logic must respond predictably
-   Results must be machine-readable and CI-compatible

This simulator mirrors that workflow by:

-   Driving deterministic state transitions
-   Injecting reproducible fault conditions
-   Enforcing safety shutdown logic
-   Producing bounded structured output
-   Supporting automated regression testing



## Industrial Relevance

This project demonstrates:

-   Control-logic validation mindset
-   Fault injection discipline
-   Deterministic simulation design
-   CI-driven automated validation
-   Defensive C programming in safety-oriented contexts
-   Clean modular lab-style architecture

It is intentionally structured to resemble a minimal validation
component that could exist inside an engine automation laboratory
pipeline.



## License

MIT License.
