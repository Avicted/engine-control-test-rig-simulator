# Message Map — BusFrame ID Registry

This document defines every frame ID in the system, its expected DLC,
encoding, and purpose.  All IDs are centrally declared in the `FrameId`
enum in `include/hal.h`.

## Frame ID Table

| ID (hex) | Enum Name            | DLC | Direction | Description                        |
| -------- | -------------------- | --- | --------- | ---------------------------------- |
| 0x100    | FRAME_ID_SENSOR      | 8   | RX        | Primary sensor data frame          |
| 0x10E    | FRAME_ID_SENSOR_ERROR| 1   | RX        | Sensor error / diagnostic frame    |
| 0x110    | FRAME_ID_SENSOR_TEMP_B| 2  | RX        | Redundant temperature channel B    |

## Sensor Frame (0x100) Payload Layout

| Byte | Field          | Encoding                                |
| ---- | -------------- | --------------------------------------- |
| 0–1  | RPM            | uint16 big-endian, raw RPM              |
| 2–3  | Temperature    | uint16 big-endian, `(temp+50)*10`       |
| 4–5  | Oil Pressure   | uint16 big-endian, `oil*100`            |
| 6    | Running Flag   | 0 = stopped, 1 = running                |
| 7    | Checksum       | XOR of bytes 0–6                        |

## Sensor Error Frame (0x10E) Payload Layout

| Byte | Field       | Encoding                  |
| ---- | ----------- | ------------------------- |
| 0    | Error code  | Application-specific byte |

## Temperature Channel B Frame (0x110) Payload Layout

| Byte | Field          | Encoding                           |
| ---- | -------------- | ---------------------------------- |
| 0–1  | Temperature B  | uint16 big-endian, `(temp+50)*10`  |

## Unknown Frame Handling

Any frame with an ID not listed above is rejected by
`hal_ingest_sensor_frame()` with `STATUS_INVALID_ARGUMENT` and a
structured error diagnostic is recorded.

## DLC Validation

DLC is validated per frame ID using `hal_expected_dlc_for_id()`.
A valid ID with the wrong DLC is also rejected with
`STATUS_INVALID_ARGUMENT`.
