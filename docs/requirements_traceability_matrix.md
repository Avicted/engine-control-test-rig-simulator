# Requirements Traceability Matrix

This matrix provides a code-level mapping between requirement IDs, scenario validations, and implementation entry points.

## Requirement Definitions

| Requirement ID | Description | Primary Logic |
|---|---|---|
| REQ-ENG-001 | Persistent over-temperature leads to shutdown | `evaluate_engine()` in `src/domain/control.c` (temperature persistence path) |
| REQ-ENG-002 | Persistent low oil pressure leads to shutdown | `evaluate_engine()` in `src/domain/control.c` (oil persistence path) |
| REQ-ENG-003 | Combined high RPM + high temperature persistence leads to warning | `evaluate_engine()` in `src/domain/control.c` (combined warning path) |
| REQ-ENG-000 | Engine state machine transitions and deterministic tick update | `engine_start()`, `engine_update()`, `engine_transition_mode()` in `src/domain/engine.c` |
| REQ-ENG-CAL-001 | Calibration lifecycle - configure, reset, and default retrieval | `control_configure_calibration()`, `control_reset_calibration()` in `src/domain/control.c` |
| REQ-ENG-PHY-001 | Configurable engine plant-model physics parameters | `engine_configure_physics()`, `engine_reset_physics()` in `src/domain/engine.c` |
| REQ-ENG-WD-001 | Software watchdog timer - detect stalled sensor reads | `hal_watchdog_configure()`, `hal_watchdog_kick()`, `hal_watchdog_check()` in `src/platform/hal.c` |
| REQ-ENG-VOTE-001 | Dual-channel temperature sensor voting with tolerance check | `hal_submit_redundant_temp()`, `hal_vote_sensors()` in `src/platform/hal.c` |
| REQ-ENG-IO-001 | HAL initialization and shutdown | `hal_init()`, `hal_shutdown()` in `src/platform/hal.c` |
| REQ-ENG-IO-002 | Sensor frame ingestion and queue management | `hal_ingest_sensor_frame()` in `src/platform/hal.c` |
| REQ-ENG-IO-003 | Sensor frame decoding with timeout detection | `hal_read_sensors()` in `src/platform/hal.c` |
| REQ-ENG-IO-004 | Sensor frame encoding with checksum | `hal_encode_sensor_frame()` in `src/platform/hal.c` |
| REQ-ENG-IO-005 | Bus receive queue management | `hal_receive_bus()` in `src/platform/hal.c` |
| REQ-ENG-IO-006 | Bus transmit queue management | `hal_transmit_bus()` in `src/platform/hal.c` |
| REQ-ENG-DIAG-001 | HAL error diagnostic retrieval | `hal_get_last_error()` in `src/platform/hal.c` |
| REQ-ENG-SCRIPT-001 | Script-driven scenario ingestion and deterministic execution | `script_parser_parse_file()` in `src/scenario/script_parser.c` |

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
| REQ-ENG-001 | `control_temp_persistence_boundary`, `control_persistence_reset`, `control_evaluate_running_temp_fault`, `control_evaluate_running_oil_fault` |
| REQ-ENG-002 | `control_oil_persistence_boundary`, `control_evaluate_running_oil_fault` |
| REQ-ENG-003 | `control_combined_warning`, `control_single_tick_threshold`, `control_output_clamp_*` |
| REQ-ENG-000 | `state_illegal_transition`, `state_legal_transitions`, `state_init_starting_running`, `state_update_*`, `state_start_*`, `state_mode_str_*` |
| REQ-ENG-CAL-001 | `control_reset_calibration`, `control_configure_*` |
| REQ-ENG-PHY-001 | `physics_default`, `physics_configure`, `physics_configure_twice`, `physics_configure_invalid`, `physics_reset`, `physics_active`, `physics_affects_update` |
| REQ-ENG-WD-001 | `hal_wd_disabled_default`, `hal_wd_configure_kick`, `hal_wd_expires`, `hal_wd_too_large`, `hal_wd_disable_after` |
| REQ-ENG-VOTE-001 | `hal_vote_single`, `hal_vote_dual_agree`, `hal_vote_dual_disagree`, `hal_vote_null_out`, `hal_vote_clears` |
| REQ-ENG-IO-001 | `hal_valid_decode`, `hal_timeout`, `hal_timeout_after_last_sample` |
| REQ-ENG-IO-002 | `hal_invalid_id`, `hal_wrong_dlc`, `hal_error_frame`, `hal_ingest_null` |
| REQ-ENG-IO-003 | `hal_corrupt_payload`, `hal_fifo_order`, `hal_read_sensors_null` |
| REQ-ENG-IO-004 | `hal_encode_null_sensor`, `hal_encode_null_frame`, `hal_encode_invalid_vals` |
| REQ-ENG-IO-005 | `hal_receive_bus_valid`, `hal_receive_bus_null`, `hal_receive_bus_bad_dlc`, `hal_bus_rx_overflow` |
| REQ-ENG-IO-006 | `hal_transmit_bus_valid`, `hal_transmit_bus_null`, `hal_transmit_bus_bad_dlc` |
| REQ-ENG-DIAG-001 | `hal_last_error_init`, `hal_last_error_null` |
| REQ-ENG-SCRIPT-001 | `parser_missing_tokens`, `parser_non_numeric`, `parser_out_of_order`, `parser_corrupt_before_sensor` |
