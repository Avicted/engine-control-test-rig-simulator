# Changelog

All notable changes to the Engine Control Test Rig Simulator are documented here.
This project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) conventions.

## [1.4.6] - 2026-03-14
### Fixed
- CD -> Linux and Win64 release artifacts now work on Arch Linux hosts through wine.

## [1.4.4] - 2026-03-14

### Fixed
- Release bundle `RUNNING.txt` now documents the exact simulator, visualizer, audit, and Wine commands reviewers should use.

## [1.4.3] - 2026-03-14

### Fixed
- Linux release artifacts no longer bundle glibc and other distro system libraries that break the visualizer on non-Ubuntu hosts.

## [1.4.2] - 2026-03-14

### Fixed
- GitHub Release workflow portability and packaging fixes for Linux and Win64 artifacts.
- Win64 runtime DLL collection during release packaging.
- Headless CI release audit behavior for Win64 Wine runs.

## [1.4.0] - 2026-03-13

### Added
- GitHub Release workflow that builds and publishes Linux and Win64 runnable artifacts for both the simulator and the Raylib visualizer.
- Portable release bundle audit script and packaged verification assets so unpacked artifacts can validate simulator behavior outside the source tree.
- Local Linux and Wine-based Win64 artifact test targets, runtime dependency collection helpers, and VS Code tasks for repeatable release dry runs.

### Changed
- Release documentation moved out of the main README into a dedicated release artifacts document to keep the top-level project overview focused on architecture and verification.

## [1.3.2] - 2026-03-13

### Changed
- CI workflow simplified into a clang quality gate and focused gcc validation job, with redundant duplicate checks removed.
- `make analyze` now includes every `analyze-*` target, including valgrind.

### Fixed
- GitHub Actions no longer reruns CI on merge commits pushed to `main` after a pull request has already been validated.

## [1.3.1] - 2026-02-26

### Fixed
- Documentation consistency and accuracy updates across project docs.

## [1.3.0] - 2026-02-25

### Changed
- **255 unit tests total** (was 161): Full coverage of all unit-testable source modules including config internals, control internals, engine internals, HAL internals, logger internals, script parser internals, state machine, status, and app main entry points.
- **100% line coverage on unit-tested modules** (1355/1355 measured lines): All lines in domain, HAL, parser, config, logger, and main modules fully exercised by unit tests. Integration/orchestration files (`test_runner`, `scenario_profiles`, `scenario_report`, `scenario_catalog`, `output`) are covered by integration tests (`make test-all`), not the unit-test coverage gate. Branch coverage is not captured in the `coverage-src.info` gate.
- **Coverage gate updated in README**: Reflects 100% achieved coverage and 255 test count.

## [1.2.1] - 2026-02-25

### Fixed
- Stale `software_version` (0.3.0) in README, schema_evolution.md, and visualization/scenarios.json - now tracks `include/version.h`.
- Removed orphaned `src/reporting/version.h` (superseded by `include/version.h`).

## [1.2.0] - 2026-02-25

### Added
- **Frame ID Registry** (`FrameId` enum): Centralized frame ID declarations with `hal_frame_id_is_known()` and `hal_expected_dlc_for_id()` helpers for deterministic DLC validation.
- **Combined warning persistence in calibration**: `combined_warning_persistence_ticks` field in calibration JSON and `ControlCalibration` struct.
- **Control-law coefficient documentation**: Named constants for `compute_control_output()` bias, divisors, and coefficients.
- **Configuration module unit tests** (13 tests): `config_load_calibration_file()` and `config_load_physics_file()` coverage including missing fields, unknown keys, NULL arguments, and physics defaults.
- **MISRA deviations expanded**: 14 formally documented deviations (DEV-001 through DEV-014) with rationale and mitigation (was 5).
- **Message map** (`docs/message_map.md`): BusFrame ID registry documentation with payload layouts.
- **Error severity model** (`docs/error_severity_model.md`): Structured severity/recoverability classification reference.
- **Schema evolution policy** (`docs/schema_evolution.md`): Semantic versioning policy for JSON output schema.
- **161 unit tests total** (was 131): Expanded HAL, state machine, and new config module coverage.

### Changed
- **HAL tests**: Expanded from 38 to 53 tests - frame ID registry, DLC validation, frame staleness, additional edge cases.
- **Engine state machine tests**: Expanded from 38 to 40 tests.

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
