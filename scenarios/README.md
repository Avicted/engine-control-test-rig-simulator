# Scenario Script Files

These deterministic scripts are compatible with:

```bash
./build/testrig --script <path>
./build/testrig --script <path> --json
```

Included scripts:

- `normal_operation.txt` -> expected final result: `OK`
- `combined_warning_persistence.txt` -> expected final result: `WARNING`
- `overheat_persistent_shutdown.txt` -> expected final result: `SHUTDOWN`
- `oil_low_persistent_shutdown.txt` -> expected final result: `SHUTDOWN`

Format per line:

```text
TICK <positive_int> RPM <float> TEMP <float> OIL <float> RUN <0|1>
```

Example:

```text
TICK 1 RPM 2200 TEMP 76 OIL 3.2 RUN 1
TICK 2 RPM 2600 TEMP 80 OIL 3.1 RUN 1
```
