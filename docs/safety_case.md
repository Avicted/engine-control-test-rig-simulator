# Safety Case - Engine Control Test Rig Simulator

## 1. Purpose

This document presents a simplified safety argument for the engine control test rig simulator. It demonstrates that the safety-relevant logic is systematically validated through deterministic test scenarios with traceable evidence.

This is a simulation environment, not a deployed control system. The safety case documents the *engineering approach* to fault detection and response validation - the same approach that would scale to a production safety case under IEC 61508 or ISO 26262.

## 2. System Context

The simulator models a simplified engine control loop with three monitored parameters:

| Parameter | Sensor | Failure Mode | Consequence |
|---|---|---|---|
| Exhaust temperature | Temperature sensor | Over-temperature | Thermal damage to turbocharger, cylinder head |
| Oil pressure | Pressure transducer | Low oil pressure | Bearing seizure, crankshaft damage |
| Engine RPM | Speed sensor | Overspeed under thermal stress | Combined fatigue, mechanical failure |

## 3. Hazard Identification

| Hazard ID | Description | Severity | Safety Function |
|---|---|---|---|
| HAZ-001 | Sustained over-temperature exceeding material limits | Catastrophic | SF-001 |
| HAZ-002 | Sustained low oil pressure leading to lubrication failure | Catastrophic | SF-002 |
| HAZ-003 | Combined high RPM and high temperature causing accelerated fatigue | Major | SF-003 |
| HAZ-004 | Sensor communication failure (timeout, corruption) | Major | SF-004 |

## 4. Safety Functions

### SF-001: Temperature Persistence Shutdown

**Requirement**: REQ-ENG-001

**Behavior**: If exhaust temperature exceeds `temperature_limit` for `temp_persistence_ticks` consecutive ticks, the engine transitions to SHUTDOWN.

**Implementation**: `evaluate_engine()` in `src/domain/control.c` - increments `ENGINE_FAULT_TEMP` counter when `temperature > temperature_limit`, resets to zero when temperature drops below limit.

**Evidence**:
- Unit tests: `control_temp_persistence_boundary`, `control_persistence_reset`, `control_single_tick_threshold`
- Integration scenarios: `overheat_ge_persistence`, `thermal_runaway`
- Negative test: `overheat_lt_persistence` (below persistence window → no shutdown)

### SF-002: Oil Pressure Persistence Shutdown

**Requirement**: REQ-ENG-002

**Behavior**: If oil pressure drops below `oil_pressure_limit` for `oil_persistence_ticks` consecutive ticks, the engine transitions to SHUTDOWN.

**Implementation**: `evaluate_engine()` in `src/domain/control.c` - increments `ENGINE_FAULT_OIL_PRESSURE` counter when `oil_pressure < oil_pressure_limit`, resets on recovery.

**Evidence**:
- Unit tests: `control_oil_persistence_boundary`, `control_oil_recovery_reset`
- Integration scenarios: `oil_low_ge_persistence`, `oil_drain`
- Negative test: Normal operation scenarios where oil is within limits

### SF-003: Combined RPM+Temperature Warning

**Requirement**: REQ-ENG-003

**Behavior**: If RPM ≥ `high_rpm_warning_threshold` AND temperature ≥ `high_temp_warning_threshold` for `combined_warning_persistence_ticks` consecutive ticks, the engine transitions to WARNING.

**Implementation**: `evaluate_engine()` in `src/domain/control.c` - increments `ENGINE_FAULT_RPM_TEMP_COMBINED` counter on combined condition, resets when either parameter drops below threshold.

**Evidence**:
- Unit tests: `control_combined_warning`, `control_combined_recovery_reset`
- Integration scenarios: `combined_warning_persistence`, `high_load`
- Negative test: `normal_operation` (below thresholds → ENGINE_OK)

### SF-004: Sensor Communication Integrity

**Requirement**: REQ-ENG-IO-001 through REQ-ENG-IO-003

**Behavior**: The HAL layer detects and rejects corrupt frames (checksum mismatch), invalid frame IDs, wrong DLC, and sensor error frames. Timeouts are raised when no valid sensor data arrives within `HAL_SENSOR_TIMEOUT_TICKS`.

**Implementation**: `hal_read_sensors()`, `hal_ingest_sensor_frame()`, `decode_sensor_frame()` in `src/platform/hal.c`

**Evidence**:
- Unit tests: `hal_corrupt_payload`, `hal_invalid_id`, `hal_wrong_dlc`, `hal_error_frame`, `hal_timeout`, `hal_timeout_after_last_sample`
- Integration scenarios: Script scenarios with `FRAME CORRUPT` directives

## 5. Safety Argument Structure

```
CLAIM: The simulator correctly detects and responds to all identified hazards.
│
├── SUBCLAIM 1: Each safety function is implemented as specified.
│   └── EVIDENCE: Source code in src/domain/control.c with @requirement annotations
│
├── SUBCLAIM 2: Each safety function is tested at boundary conditions.
│   └── EVIDENCE: Unit tests exercise exact persistence thresholds (N-1, N, N+1)
│
├── SUBCLAIM 3: Fault counter reset logic prevents false shutdowns.
│   └── EVIDENCE: Recovery tests verify counters reset to zero on parameter recovery
│
├── SUBCLAIM 4: The deterministic architecture guarantees reproducible behavior.
│   └── EVIDENCE: No threads, no real-time clock, no dynamic allocation, no randomness
│
├── SUBCLAIM 5: Static analysis confirms code quality.
│   └── EVIDENCE: cppcheck + clang-tidy + MISRA addon with zero unaddressed warnings
│
└── SUBCLAIM 6: Requirement traceability connects hazards → requirements → tests.
    └── EVIDENCE: requirements_traceability_matrix.md maps REQ-ENG-* to test evidence
```

## 6. Residual Risk

| Risk | Mitigation | Status |
|---|---|---|
| Engine physics model is simplified | Clearly documented as simulation, not plant model | Accepted |
| No RTOS/interrupt modeling | Tick-based determinism covers logical correctness; timing analysis is out of scope | Accepted |
| Single sensor channel (no redundancy) | Identified as future enhancement (dual-sensor voting) | Open |
| Calibration values are trusted after validation | `calibration_valid()` enforces bounds; production system would use CRC-protected NVM | Accepted |

## 7. Conclusion

The safety functions in this simulator are systematically specified, implemented with defensive coding practices, tested at boundary conditions, and traced from hazard identification through requirements to test evidence. The deterministic architecture and CI quality gate ensure that safety-relevant behavior is reproducible and regression-protected.
