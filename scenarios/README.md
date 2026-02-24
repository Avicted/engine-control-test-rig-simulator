# Scenario Script Files

These deterministic scripts are compatible with:

```bash
./build/testrig --script <path>
./build/testrig --script <path> --json
```

Included scripts:

- `normal_operation.txt` -> expected final state: `OK`
- `combined_warning_persistence.txt` -> expected final state: `WARNING`
- `overheat_persistent_shutdown.txt` -> expected final state: `SHUTDOWN`
- `oil_low_persistent_shutdown.txt` -> expected final state: `SHUTDOWN`

Longer scripts (up to 25 ticks):

- `cold_start_warmup_and_ramp.txt` -> expected final state: `OK`
  - Cold start from 800 RPM, gradual warm-up through the full operating range, cruise at 3400 RPM, then a symmetric ramp-down. All sensor values remain safely within thresholds throughout.
- `high_load_warning_then_recovery.txt` -> expected final state: `WARNING`
  - Ramp to high load where RPM ≥ 3500 and TEMP ≥ 85 holds for two consecutive ticks, triggering the combined-fault WARNING. The engine then decelerates back to safe values, but the state machine keeps the engine in WARNING.
- `oil_pressure_gradual_drain.txt` -> expected final state: `SHUTDOWN`
  - All values are healthy while oil pressure drains slowly from 3.5 bar toward critical. Once oil pressure crosses below 2.5 bar for three consecutive ticks the evaluation triggers SHUTDOWN, and the fourth fault tick completes the state transition.
- `thermal_runaway_with_load_surge.txt` -> expected final state: `SHUTDOWN`
  - Engine sustains a heavy load at 3400 RPM while temperature climbs steadily from 68 °C past the critical threshold of 95 °C. Three consecutive over-temperature ticks trigger the SHUTDOWN evaluation, and the fourth tick completes the state transition.
- `intermittent_oil_then_combined_fault.txt` -> expected final state: `WARNING`
  - Oil pressure briefly dips below 2.5 bar for two ticks (counter reaches 2) then recovers, resetting the counter before shutdown can occur. A subsequent load surge pushes RPM ≥ 3500 and TEMP ≥ 85 for two consecutive ticks, triggering a combined-fault WARNING that the engine does not recover from.

Format per line:

```text
TICK <positive_int> RPM <float> TEMP <float> OIL <float> RUN <0|1>
```

Example:

```text
TICK 1 RPM 2200 TEMP 76 OIL 3.2 RUN 1
TICK 2 RPM 2600 TEMP 80 OIL 3.1 RUN 1
```
