# Failure Mode and Effects Analysis (FMEA)

## Scope

This FMEA covers sensor-level and communication-level failure modes in the engine control test rig simulator. It maps each failure mode to the system response and provides test evidence.

## Severity / Occurrence / Detectability Scale

| Rating | Severity | Occurrence | Detectability |
|---|---|---|---|
| 1 | Negligible | Remote | Almost certain detection |
| 2 | Minor | Low | High detection |
| 3 | Moderate | Moderate | Moderate detection |
| 4 | Major | High | Low detection |
| 5 | Catastrophic | Very high | Undetectable |

## FMEA Table

| ID | Component | Failure Mode | Local Effect | System Effect | Severity | Occurrance | Detection | Risk Priority Number | Detection Mechanism | System Response | Test Evidence |
|---|---|---|---|---|---|---|---|---|---|---|---|
| FM-001 | Temperature sensor | Stuck high (above limit) | Sustained over-temp reading | Persistence counter increments continuously | 4 | 2 | 1 | 8 | `evaluate_engine()` fault counter | SHUTDOWN after persistence threshold | `overheat_ge_persistence`, `control_temp_persistence_boundary` |
| FM-002 | Temperature sensor | Stuck low (below limit) | Normal-range reading despite actual fault | No shutdown triggered (masked fault) - mitigated by dual-channel voting | 4 | 2 | 1 | 8 | Dual-channel sensor voting via `hal_vote_sensors()` detects disagreement | Cross-check disagreement triggers STATUS_SENSOR_MISMATCH | `hal_vote_dual_disagree`, `hal_vote_single`, `hal_vote_dual_agree` |
| FM-003 | Temperature sensor | Intermittent spike | Counter increments then resets | No shutdown if spikes don't persist | 2 | 3 | 1 | 6 | Counter reset on recovery in `update_fault_counter()` | Correct: no false shutdown | `control_persistence_reset` |
| FM-004 | Oil pressure sensor | Stuck low (below limit) | Sustained low-oil reading | Persistence counter increments | 4 | 2 | 1 | 8 | `evaluate_engine()` fault counter | SHUTDOWN after persistence threshold | `oil_low_ge_persistence`, `control_oil_persistence_boundary` |
| FM-005 | Oil pressure sensor | Gradual decay | Slow decrease over many ticks | SHUTDOWN triggered when persistence window met | 4 | 3 | 1 | 12 | Counter tracks consecutive low ticks | SHUTDOWN | `oil_drain` scenario |
| FM-006 | Oil pressure sensor | Intermittent recovery | Counter resets on recovery ticks | No shutdown if recovery resets counter | 2 | 3 | 1 | 6 | Counter reset in `update_fault_counter()` | Correct behavior | `control_oil_recovery_reset`, `intermittent_oil` |
| FM-007 | CAN bus | Frame corruption (bit error) | Checksum mismatch in `BusFrame` | Frame rejected, STATUS_PARSE_ERROR | 3 | 3 | 1 | 9 | XOR checksum in `frame_checksum()` | Frame rejected, sensor timeout if persistent | `hal_corrupt_payload` |
| FM-008 | CAN bus | Wrong frame ID | Frame ID not in supported set | Frame rejected at ingestion | 2 | 2 | 1 | 4 | `is_supported_sensor_transport_frame()` | STATUS_INVALID_ARGUMENT | `hal_invalid_id` |
| FM-009 | CAN bus | Wrong DLC | DLC mismatch for frame type | Frame rejected at ingestion | 2 | 2 | 1 | 4 | DLC check in `is_supported_sensor_transport_frame()` | STATUS_INVALID_ARGUMENT | `hal_wrong_dlc` |
| FM-010 | CAN bus | Complete loss of communication | No frames arrive | Sensor timeout after `HAL_SENSOR_TIMEOUT_TICKS` | 4 | 2 | 1 | 8 | Tick-based timeout in `hal_read_sensors()` | STATUS_TIMEOUT (SEVERITY_FATAL) | `hal_timeout` |
| FM-011 | CAN bus | Delayed communication | Frames arrive late but eventually | Timeout if gap exceeds threshold | 3 | 2 | 1 | 6 | `g_last_sensor_tick` tracking in `hal_read_sensors()` | STATUS_TIMEOUT if beyond threshold | `hal_timeout_after_last_sample` |
| FM-012 | Sensor ECU | Error frame transmitted | Error frame ID (0x10E) received | Frame queued, decoded as error | 3 | 2 | 1 | 6 | Frame ID detection in `hal_read_sensors()` | STATUS_PARSE_ERROR | `hal_error_frame` |
| FM-013 | Bus buffer | Queue overflow | More frames than buffer capacity (32) | New frames rejected | 3 | 1 | 1 | 3 | Queue count check in `queue_push()` | STATUS_BUFFER_OVERFLOW | `hal_bus_rx_overflow` |
| FM-014 | RPM + Temp combined | High RPM under thermal stress | Both thresholds exceeded simultaneously | Combined warning counter increments | 3 | 2 | 1 | 6 | `evaluate_engine()` combined fault counter | WARNING after persistence | `control_combined_warning` |
| FM-015 | Calibration | Invalid calibration values | Out-of-range thresholds or zero persistence | Configuration rejected | 4 | 1 | 1 | 4 | `calibration_valid()` boundary checks | STATUS_INVALID_ARGUMENT | `control_configure_invalid_temp`, `control_cal_*` tests |

## Risk Priority Number (RPN) Summary

| RPN Range | Count | Action |
|---|---|---|
| 1-9 | 12 | Acceptable risk - detection mechanism verified by test |
| 10-19 | 2 | Low risk - monitored, tested |
| 20-39 | 0 | None (FM-002 mitigated by dual-channel voting, RPN reduced from 32 to 8) |
| 40+ | 0 | None |

## High-RPN Item Analysis

### FM-002: Temperature Sensor Stuck Low (RPN 8, mitigated)

**Analysis**: If the physical temperature sensor reports falsely low values, a single-channel design cannot detect the discrepancy. The dual-channel sensor voting mechanism (`hal_submit_redundant_temp()` / `hal_vote_sensors()`) now cross-checks channel A against channel B with a configurable tolerance (`HAL_SENSOR_VOTING_TOLERANCE`). Disagreement beyond the tolerance is detected and flagged.

**Mitigation**: Dual-sensor voting implemented in v1.1.0 (REQ-ENG-VOTE-001). Cross-check disagreement detection tested via `hal_vote_dual_disagree`. Detectability reduced from 4 to 1, RPN reduced from 32 to 8.

## Revision History

| Version | Date | Author | Change |
|---|---|---|---|
| 1.0 | 2026-02-25 | Victor Anderssén | Initial FMEA covering sensor and bus failure modes |
| 1.1 | 2026-02-25 | Victor Anderssén | Updated FM-002: dual-channel voting implemented (REQ-ENG-VOTE-001), RPN reduced 32→8 |
