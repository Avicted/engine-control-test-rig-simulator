# Requirements Traceability Matrix

This matrix provides a code-level mapping between requirement IDs, scenario validations, and implementation entry points.
The canonical requirement-ID registry is `include/requirements.h`.

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
| REQ-ENG-IO-007 | Frame ID registry DLC lookup | `hal_expected_dlc_for_id()` in `src/platform/hal.c` |
| REQ-ENG-IO-008 | Frame staleness tracking and query | `hal_frame_age_record()`, `hal_frame_is_stale()` in `src/platform/hal.c` |
| REQ-ENG-DIAG-001 | HAL error diagnostic retrieval | `hal_get_last_error()` in `src/platform/hal.c` |

## Supporting Traceability Definitions

These requirement IDs are also traced in code and defined in `include/requirements.h`, but they are infrastructure-
level entry points rather than scenario `requirement_id` values emitted for the named scenario catalog.

| Requirement ID | Description | Primary Logic |
|---|---|---|
| REQ-ENG-TEST-001 | Run full deterministic scenario registry | `run_all_tests*()` in `include/test_runner.h` / `src/app/test_runner.c` |
| REQ-ENG-TEST-002 | Run a named deterministic scenario | `run_named_scenario*()` in `include/test_runner.h` / `src/app/test_runner.c` |
| REQ-ENG-JSON-001 | Emit deterministic JSON scenario report envelope and per-scenario requirement linkage | `run_all_tests_with_json()`, `run_named_scenario_with_json()`, `scenario_report_print_json_header()` |
| REQ-ENG-JSON-002 | Emit deterministic JSON scenario report footer and summary | `scenario_report_print_json_footer()` in `src/scenario/scenario_report.c` |
| REQ-ENG-DIAG-002 | Emit structured JSON diagnostic error object | `scenario_report_print_json_error()` in `src/scenario/scenario_report.c` |
| REQ-ENG-SCRIPT-001 | Parse scripted scenario files and emit the runtime-traced scripted scenario requirement ID | `script_parser_parse_file()`, `run_scripted_scenario_with_json()` |
| REQ-ENG-SCRIPT-002 | Execute scripted scenario frame profiles through the deterministic HAL path | `execute_profile_frames()` in `src/scenario/scenario_profiles.c` |

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
| scripted_scenario | REQ-ENG-SCRIPT-001 | `run_scripted_scenario_with_json()` | Runtime-evaluated (deterministic) |

## Runtime and Integration Evidence Mapping

| Requirement ID | Runtime / Integration Evidence |
|---|---|
| REQ-ENG-000 | Named scenario execution via `run_all_tests_with_json()` and scripted execution via `run_scripted_scenario_with_json()` both drive `engine_start()` and repeated `engine_update()` transitions. |
| REQ-ENG-001 | Named scenarios `overheat_ge_persistence`, `thermal_runaway` and scripted scenarios `overheat_persistent_shutdown`, `cold_start_warmup_and_ramp_high_temp_shutdown` exercise persistent over-temperature shutdown behavior. |
| REQ-ENG-002 | Named scenarios `oil_low_ge_persistence`, `oil_drain`, `intermittent_oil` and scripted scenarios `oil_low_persistent_shutdown`, `oil_pressure_gradual_drain` exercise persistent low-oil shutdown behavior. |
| REQ-ENG-003 | Named scenarios `normal_operation`, `combined_warning_persistence`, `high_load`, `cold_start` and scripted scenarios `combined_warning_persistence`, `high_load_warning_then_recovery`, `normal_operation` exercise the combined warning path. |
| REQ-ENG-CAL-001 | Runtime configuration path `app_main_entry(... --config <file> ...)` applies calibration through `apply_runtime_options()` before `--run-all`, `--scenario`, and `--script` execution. |
| REQ-ENG-PHY-001 | Named and scripted scenarios execute through `engine_update()` with the active plant model, providing integration evidence for the physics path while unit tests cover configure/reset behavior. |
| REQ-ENG-WD-001 | Dedicated HAL watchdog verification path is exercised in deterministic module/integration tests; no CLI-named scenario currently exposes watchdog configuration directly. |
| REQ-ENG-VOTE-001 | Dedicated HAL redundant-temperature voting sequence is exercised in deterministic module/integration tests; no CLI-named scenario currently injects dual-channel disagreement directly. |
| REQ-ENG-IO-001 | `run_all_tests_with_json()`, `run_named_scenario_with_json()`, and `run_scripted_scenario_with_json()` all call `hal_init()` / `hal_shutdown()` around scenario execution. |
| REQ-ENG-IO-002 | Scripted scenario execution and `validate-scripted-scenarios` drive parsed sensor frames through `hal_ingest_sensor_frame()`. |
| REQ-ENG-IO-003 | Scripted scenario execution drives `hal_read_sensors()` decode and timeout handling through `execute_profile_frames()`. |
| REQ-ENG-IO-004 | Script parser execution and `validate-scripted-scenarios` encode deterministic sensor frames before scripted execution. |
| REQ-ENG-IO-005 | Dedicated HAL bus-receive queue integration is exercised in direct module tests; no scenario-catalog route currently consumes the RX bus API. |
| REQ-ENG-IO-006 | Dedicated HAL bus-transmit queue integration is exercised in direct module tests; no scenario-catalog route currently consumes the TX bus API. |
| REQ-ENG-IO-007 | Scripted frame ingestion validates registered frame IDs and DLC expectations during parser/execution flow, with dedicated direct lookup coverage in module tests. |
| REQ-ENG-IO-008 | Frame-age record/query integration is exercised in deterministic module tests; no scenario-catalog route currently models stale-frame aging. |
| REQ-ENG-DIAG-001 | Scripted scenario error handling retrieves HAL diagnostics through `hal_get_last_error()` when profile execution fails. |
| REQ-ENG-DIAG-002 | `validate-json-error-contract` drives a failing scripted scenario through JSON emission and verifies the structured `error` object emitted by `scenario_report_print_json_error()`. |
| REQ-ENG-TEST-001 | `run_all_tests_with_json()` and `make test-all` execute the full deterministic named-scenario registry and validate its summary output. |
| REQ-ENG-TEST-002 | `run_named_scenario_with_json()` and the CLI `--scenario` path execute a selected named scenario with traced `requirement_id` output. |
| REQ-ENG-JSON-001 | `validate-json-contract`, `validate-json`, and named/scripted JSON execution paths verify top-level metadata and per-scenario `requirement_id` linkage. |
| REQ-ENG-JSON-002 | `validate-json-contract`, `validate-json`, and scripted error JSON validation verify deterministic summary/footer emission. |
| REQ-ENG-SCRIPT-001 | `validate-scripted-scenarios` and CLI `--script` execution verify script parsing, runtime execution, and emitted scripted-scenario `requirement_id`. |
| REQ-ENG-SCRIPT-002 | `validate-scripted-scenarios` and visualization bundle generation execute scripted frame profiles through `execute_profile_frames()`. |

## Named Scenario Routing

Named scenario routing in `run_named_scenario_with_json()` uses `scenario_catalog_find_named()` and `selected_test->requirement_id` to assign `requirement_id` for JSON output.

## Evidence Outputs

- Console output: requirement linkage per test case.
- JSON output: `requirement_id`, `expected`, `actual`, and `pass` per scenario.
- CI gates: `make validate-json-contract`, `make validate-scripted-scenarios`, `make validate-runtime-config`, and `make validate-json-error-contract` verify runtime JSON/report behavior.
- Traceability gate: `make validate-requirements` verifies registry completeness and that every `REQ-ENG-*` has runtime/integration and test evidence in this matrix.

## Test Evidence Mapping

| Requirement ID | Unit Test Evidence |
|---|---|
| REQ-ENG-001 | `control_temp_persistence_boundary`, `control_persistence_reset`, `control_single_tick_threshold` |
| REQ-ENG-002 | `control_oil_persistence_boundary`, `control_oil_recovery_reset` |
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
| REQ-ENG-IO-007 | `hal_fid_known_valid`, `hal_dlc_sensor`, `hal_dlc_unknown_id`, `hal_dlc_mismatch_ingest` |
| REQ-ENG-IO-008 | `hal_age_fresh_receipt`, `hal_age_stale_boundary`, `hal_age_never_recv`, `hal_age_unknown_id` |
| REQ-ENG-DIAG-001 | `hal_last_error_init`, `hal_last_error_null`, scripted execution error-handling path in `run_scripted_scenario_with_json()` |
| REQ-ENG-DIAG-002 | `validate-json-error-contract`, `validate-json` |
| REQ-ENG-TEST-001 | `main_entry_run_all`, `validate-json-contract`, `test-all` |
| REQ-ENG-TEST-002 | `main_entry_scenario`, `run_named_scenario_with_json()` integration path |
| REQ-ENG-JSON-001 | `validate-json-contract`, `validate-json`, `validate-scripted-scenarios` |
| REQ-ENG-JSON-002 | `validate-json-contract`, `validate-json`, `validate-json-error-contract` |
| REQ-ENG-SCRIPT-001 | `parser_missing_tokens`, `parser_non_numeric`, `parser_out_of_order`, `parser_corrupt_before_sensor`, `main_entry_script`, `validate-scripted-scenarios` |
| REQ-ENG-SCRIPT-002 | `validate-scripted-scenarios`, visualization bundle generation via `tools/generate_visualization_scenario_json.py` |
