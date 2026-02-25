# Requirements Traceability Matrix

This matrix provides a code-level mapping between requirement IDs, scenario validations, and implementation entry points.

## Requirement Definitions

| Requirement ID | Description | Primary Logic |
|---|---|---|
| REQ-ENG-001 | Persistent over-temperature leads to shutdown | `evaluate_engine()` in `src/domain/control.c` (temperature persistence path) |
| REQ-ENG-002 | Persistent low oil pressure leads to shutdown | `evaluate_engine()` in `src/domain/control.c` (oil persistence path) |
| REQ-ENG-003 | Combined high RPM + high temperature persistence leads to warning | `evaluate_engine()` in `src/domain/control.c` (combined warning path) |
| REQ-ENG-SCRIPT | Script-driven scenario ingestion contract and deterministic execution | `script_parser_parse_file()` in `src/scenario/script_parser.c`, `run_scripted_scenario_with_json()` in `src/app/test_runner.c` |

## Scenario-to-Requirement Mapping

| Scenario Name | Requirement ID | Scenario Function | Expected Result |
|---|---|---|---|
| normal_operation | REQ-ENG-003 | `scenario_normal_operation()` | ENGINE_OK |
| overheat_lt_persistence | REQ-ENG-001 | `scenario_overheat_short_duration()` | ENGINE_OK |
| overheat_ge_persistence | REQ-ENG-001 | `scenario_overheat_persistent()` | ENGINE_SHUTDOWN |
| oil_low_ge_persistence | REQ-ENG-002 | `scenario_oil_pressure_persistent()` | ENGINE_SHUTDOWN |
| combined_warning_persistence | REQ-ENG-003 | `scenario_combined_warning_persistent()` | ENGINE_WARNING |
| cold_start | REQ-ENG-003 | `scenario_cold_start_warmup_and_ramp()` | ENGINE_OK |
| high_load | REQ-ENG-003 | `scenario_high_load_warning_then_recovery()` | ENGINE_WARNING |
| oil_drain | REQ-ENG-002 | `scenario_oil_pressure_gradual_drain()` | ENGINE_SHUTDOWN |
| thermal_runaway | REQ-ENG-001 | `scenario_thermal_runaway_with_load_surge()` | ENGINE_SHUTDOWN |
| intermittent_oil | REQ-ENG-002 | `scenario_intermittent_oil_then_combined_fault()` | ENGINE_WARNING |
| scripted_scenario | REQ-ENG-SCRIPT | `run_scripted_scenario_with_json()` | Runtime-evaluated (deterministic) |

## Named Scenario Routing

Named scenario routing in `run_named_scenario_with_json()` uses `requirement_for_named_scenario()` to assign `requirement_id` for JSON output.

## Evidence Outputs

- Console output: requirement linkage per test case.
- JSON output: `requirement_id`, `expected`, `actual`, and `pass` per scenario.
- CI gate: `make validate-json-contract` ensures required JSON envelope and fields exist.

## Unit-Test Evidence Mapping

| Requirement ID | Unit Test Evidence |
|---|---|
| REQ-ENG-001 | `control_temp_persistence_boundary`, `control_persistence_reset` |
| REQ-ENG-002 | `control_oil_persistence_boundary` |
| REQ-ENG-003 | `control_combined_warning`, `control_single_tick_threshold` |
| REQ-ENG-IO-001 | `hal_valid_decode`, `hal_timeout`, `hal_timeout_after_last_sample` |
| REQ-ENG-IO-002 | `hal_invalid_id`, `hal_wrong_dlc`, `hal_error_frame` |
| REQ-ENG-IO-003 | `hal_corrupt_payload`, `hal_fifo_order` |
| REQ-ENG-SCRIPT | `parser_missing_tokens`, `parser_non_numeric`, `parser_out_of_order`, `parser_corrupt_before_sensor` |
