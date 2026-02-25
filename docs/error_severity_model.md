# Error Severity Model

## Overview

All errors in the system carry structured metadata:

```c
typedef struct {
    StatusCode     code;           // What happened
    const char    *module;         // Which module
    const char    *function;       // Which function
    uint32_t       tick;           // When (simulation tick)
    Severity       severity;       // How serious
    Recoverability recoverability; // Can the system continue?
} ErrorInfo;
```

## Severity Levels

| Severity    | Value | Meaning                        | Action                          |
| ----------- | -----:| ------------------------------ | ------------------------------- |
| INFO        | 0     | Normal operation               | None                            |
| WARNING     | 1     | Condition requires attention   | Log, may recover autonomously   |
| ERROR       | 2     | Recoverable fault              | Report, attempt recovery        |
| FATAL       | 3     | Non-recoverable fault          | Force SHUTDOWN, abort scenario  |

## Recoverability

| Classification      | Meaning                               | Default For                      |
| ------------------- | ------------------------------------- | -------------------------------- |
| RECOVERABLE         | System can continue autonomously      | Most status codes                |
| NON_RECOVERABLE     | Manual intervention required          | `STATUS_INTERNAL_ERROR`, `STATUS_IO_ERROR` |

## Fatal Conditions

The following conditions are classified as **FATAL** and trigger
deterministic engine shutdown:

| Condition              | Module  | StatusCode         | Recovery                |
| ---------------------- | ------- | ------------------ | ----------------------- |
| Sensor timeout         | HAL     | STATUS_TIMEOUT     | Engine transitions to SHUTDOWN |
| Watchdog expiry        | HAL     | STATUS_TIMEOUT     | Engine transitions to SHUTDOWN |
| Illegal state machine  | Domain  | STATUS_INTERNAL_ERROR | Scenario aborts      |

## Free-Form Error Policy

**No free-form `printf` error messages are permitted.**
Every error path must call `hal_set_error()` or return a `StatusCode`.
CI enforces `-Werror` and `-Wunused-result` to prevent silent errors.

## Queue Overflow Policy

Overflow uses the **DROP NEWEST** policy:
- New frames are rejected with `STATUS_BUFFER_OVERFLOW`.
- A structured error diagnostic is recorded.
- Existing queue contents are preserved.

See `include/hal.h` for detailed documentation.
