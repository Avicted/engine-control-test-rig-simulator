# Changelog

All notable changes to the Engine Control Test Rig Simulator are documented here.
This project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) conventions.

## [1.1.0] - 2025-01-25

### Added
- **Sensor voting / dual-channel redundancy** (`REQ-ENG-VOTE-001`): `hal_submit_redundant_temp()` / `hal_vote_sensors()` for dual-channel temperature cross-check with configurable tolerance.
- **Software watchdog timer** (`REQ-ENG-WD-001`): `hal_watchdog_configure()` / `hal_watchdog_kick()` / `hal_watchdog_check()` to detect stalled sensor reads and force SHUTDOWN.
- **Engine physics parameterization** (`REQ-ENG-PHY-001`): `EnginePhysicsConfig` struct with configurable target RPM, temperature, oil pressure, and ramp rates - separates plant model from controller.
- **Calibration lifecycle reset** (`REQ-ENG-CAL-001`): `control_reset_calibration()` for unit test isolation.
- **Physics configuration in calibration.json**: Optional `"physics"` section with schema validation.
- **MISRA C:2012 analysis** (`make analyze-misra`): cppcheck addon with rule classification and documented deviations (DEV-001 through DEV-005).
- **Doxygen-formatted API documentation**: All public headers converted to `/** */` with `@param`, `@return`, `@retval`, `@requirement` annotations. Includes `Doxyfile` and `make docs` target.
- **Safety case** (`docs/safety_case.md`): Structured safety argument with hazard identification (HAZ-001-004), safety functions (SF-001-004), and residual risk table.
- **FMEA** (`docs/fmea.md`): 15-item Failure Mode and Effects Analysis with Severity/Occurrence/Detectability ratings and RPN analysis.
- **Architecture Decision Records**: ADR-001 (zero dynamic allocation), ADR-002 (deterministic tick simulation), ADR-003 (layered dependency enforcement), ADR-004 (JSON schema output).
- **MISRA deviations log** (`docs/misra_deviations.md`): 5 formally documented deviations with justification and mitigation.
- **Logger unit tests** (15 tests): Full coverage of `logger_set_level`, `log_event`, `logger_log_tick`, CI suppression, color modes.
- **131 unit tests total** (was 23): Comprehensive coverage of all modules.

### Changed
- **Makefile**: Fixed corruption in `run-script` / `run-script-json` / `run-scenarios` targets (leaked `$(CC)` rule bodies).
- **Coverage gate**: Rewritten to compute aggregate of source files only (`src/`), excluding test files. Honest 80% threshold.
- **`engine_update()`**: Now uses `EnginePhysicsConfig` instead of hard-coded constants (3000 RPM, 90°C, 3.4 bar).
- **Control module tests**: Expanded from 5 to 33 tests - covers all `evaluate_engine` branches, calibration validation, fault counter recovery.
- **Engine state machine tests**: Expanded from 3 to 38 tests - exhaustive illegal transition pairs, physics config lifecycle.
- **HAL tests**: Expanded from 8 to 38 tests - encode/decode, bus queues, actuators, watchdog, sensor voting.

### Fixed
- Makefile target `run-script` / `run-script-json` / `run-scenarios` had leaked `$(CC)` compilation lines from merge artifact.
- Coverage gate was reporting ~97% by measuring a test file instead of source files.

## [1.0.0] - Initial Release

### Added
- Deterministic engine control loop simulator with tick-based execution.
- Five-state engine state machine (INIT → STARTING → RUNNING → WARNING → SHUTDOWN).
- CAN-like HAL with frame encode/decode, bounded FIFO queues, sensor timeout detection.
- Safety evaluation with fault persistence counters (temperature, oil pressure, combined RPM+temp).
- Control output computation with [0, 100] clamping.
- Script-driven test scenarios with monotonic tick validation.
- JSON Schema-validated calibration and scenario output.
- Layered architecture with `tools/check_layering.sh` enforcement.
- Integration test scenarios (cold start, overheat, oil drain, thermal runaway, etc.).
- `ci-check` target orchestrating build, analysis, test, and coverage gates.
