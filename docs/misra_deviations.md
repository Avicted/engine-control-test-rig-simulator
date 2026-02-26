# MISRA C:2012 Deviation Record

This document records intentional deviations from MISRA C:2012 rules in the
engine-control test-rig simulator. The project enforces compliance via
`make analyze-misra` which runs **cppcheck** with the MISRA C:2012 addon
and `--error-exitcode=1`. Suppressed rules are listed in the Makefile; any
violation of a non-suppressed rule **fails the build**.

> **Edition note (2026-02)**: MISRA C:2025 support in common open-source static
> analysis toolchains is not assumed here. This project targets MISRA C:2012 and
> can migrate when the chosen toolchain supports it.

---

## Rules Fixed in Code

The following rules had violations that were **eliminated by code changes** -
no deviations required:

| Rule | Category | Violations Fixed | Fix Applied |
|------|----------|-----------------|-------------|
| 11.4 | Advisory | 167 | Replaced `(type *)0` with `NULL`; `_Static_assert` uses compound literals |
| 11.8 | Required | 2 | Removed unnecessary `(char *)` cast on `strstr()` return |
| 12.1 | Advisory | 7 | Added explicit parentheses around mixed-precedence expressions |
| 13.4 | Advisory | 2 | Hoisted `while ((x = f()) != ...)` assignments before the loop |
| 14.2 | Required | 2 | Replaced `index += 1` in for-body with `skip_next` flag |
| 15.7 | Required | 1 | Added terminal `else` clause |

---

## Deviation Summary

| ID | Rule | Category | Suppressed Count | Scope |
|----|------|----------|-----------------|-------|
| DEV-001 | 15.5 | Advisory | 383 | Project-wide - early-return guard pattern |
| DEV-002 | 21.6 | Required | 81 | `src/` - `stdio.h` functions |
| DEV-003 | 8.7 | Advisory | 20 | `src/` - external-linkage functions testable from separate TU |
| DEV-004 | 10.4 | Required | 11 | `include/` - `_Static_assert` operand type mismatch |
| DEV-005 | 18.4 | Advisory | 10 | `src/` - pointer arithmetic on arrays/buffers |
| DEV-006 | 11.5 | Advisory | 10 | `src/` - conversion from `void *` to object pointer |
| DEV-007 | 4.1 | Required | 8 | `src/reporting/` - ANSI colour escape sequences |
| DEV-008 | 10.8 | Required | 3 | `src/platform/hal.c` - composite expression widening cast |
| DEV-009 | 8.9 | Advisory | 2 | `src/domain/` - file-scope static data |
| DEV-010 | 2.5 | Advisory | 2 | `include/` - unused macro definitions |
| DEV-011 | 2.3 | Advisory | 2 | `include/` - unused type declarations |
| DEV-012 | 2.4 | Advisory | 1 | `include/` - unused tag declaration |
| DEV-013 | 21.8 | Required | 1 | `src/reporting/logger.c` - `getenv("CI")` |
| DEV-014 | 1.2 | Advisory | - | `include/hal.h` - `__attribute__((packed))` |

---

## Detailed Deviations

### DEV-001: Rule 15.5 - A function should have a single point of exit at the end

**Category**: Advisory
**Suppressed count**: 383
**Locations**: All source files

**Rationale**: The project uses a **guard-clause / early-return** coding style
where each public function begins with NULL-pointer checks and argument
validation that return immediately on failure. This pattern is widely adopted in
safety-critical C (e.g., AUTOSAR, NASA/JPL) because it keeps the happy path at
the lowest indentation level and makes error handling explicit at the call site.
Converting 383 early returns into a single-exit structure would require deeply
nested if/else or goto-cleanup patterns that are harder to review.

**Mitigation**: Every early-return path is unit-tested (255 tests, 100% line coverage on unit-tested modules). Functions that return error codes are documented with
`@return` Doxygen annotations.

---

### DEV-002: Rule 21.6 - The Standard Library I/O functions shall not be used

**Category**: Required
**Suppressed count**: 81
**Locations**: `src/scenario/script_parser.c`, `src/app/config.c`,
`src/reporting/output.c`, `src/reporting/logger.c`,
`src/scenario/scenario_report.c`

**Rationale**: The simulator is a **host-side test harness**, not a deployed
embedded controller. It must read scenario scripts from files (`fopen`/`fgets`),
parse calibration JSON (`sscanf`, `strtof`, `strtoul`), write diagnostic logs
(`fprintf`), and emit JSON contract reports (`snprintf`, `fwrite`). There is no
alternative I/O mechanism on the host platform.

**Mitigation**: All format strings are compile-time constants. Buffer sizes are
bounded by `sizeof` or `_Static_assert`. Parsed values pass through range
validation. File handles are closed deterministically.

---

### DEV-003: Rule 8.7 - Functions/objects should not be external if referenced in only one TU

**Category**: Advisory
**Suppressed count**: 20
**Locations**: Various `src/` files

**Rationale**: Functions exposed in public headers (`engine.h`, `control.h`,
etc.) are intentionally extern to support unit testing from a separate
translation unit (`tests/`). Making them `static` would break the test harness.

**Mitigation**: The `analyze-layering` target validates include-graph
dependencies. Only the declared public API crosses module boundaries.

---

### DEV-004: Rule 10.4 - Both operands shall have the same essential type category

**Category**: Required
**Suppressed count**: 11
**Locations**: `include/engine.h`, `include/hal.h` (`_Static_assert` expressions)

**Rationale**: `_Static_assert` comparisons like `sizeof(...) == 4U` involve
`size_t` compared with an unsigned literal. Both operands are unsigned; the
mismatch is a cppcheck false positive. The assertion generates no runtime code.

**Mitigation**: These are compile-time-only checks. Operands are always unsigned
constants.

---

### DEV-005: Rule 18.4 - Pointer arithmetic should not be used

**Category**: Advisory
**Suppressed count**: 10
**Locations**: `src/app/config.c`, `src/scenario/script_parser.c`,
`src/platform/hal.c`

**Rationale**: Pointer arithmetic is used for bounded buffer traversal during
JSON key parsing (`cursor + 1`, `end_quote + 1`) and CAN frame byte
encoding/decoding. These operations always work within a known-size buffer whose
bounds are checked before access.

**Mitigation**: All pointer arithmetic is bounded by prior length checks.
AddressSanitizer (`make analyze-sanitizers`) validates memory access at runtime.

---

### DEV-006: Rule 11.5 - Conversion from pointer to void to pointer to object

**Category**: Advisory
**Suppressed count**: 10
**Locations**: `src/scenario/scenario_profiles.c`, `src/app/test_runner.c`

**Rationale**: The test framework passes scenario-specific context through a
`void *` parameter in the generic `TestCase` function signature. Each scenario
function casts this back to the expected concrete type. This is the standard C
pattern for type-erased callbacks.

**Mitigation**: NULL-pointer checks precede every cast. The type relationship is
enforced by the scenario registration code that passes the context.

---

### DEV-007: Rule 4.1 - Octal and hexadecimal escape sequences shall be terminated

**Category**: Required
**Suppressed count**: 8
**Locations**: `src/reporting/logger.c`

**Rationale**: ANSI terminal colour codes use hex escape sequences (e.g.,
`\x1b[31m`). These are string constants consumed only by terminal emulators and
are gated behind the `--color` CLI flag.

**Mitigation**: Colour output is disabled by default and in CI. The escape
sequences are compile-time string literals.

---

### DEV-008: Rule 10.8 - Composite expression cast to wider essential type

**Category**: Required
**Suppressed count**: 3
**Locations**: `src/platform/hal.c`

**Rationale**: CAN frame encoding casts `float` arithmetic results to
`uint16_t` for wire format. The cast is explicit and intentional, with
intermediate values bounded by physical constraints (RPM < 65535, temperature
offset < 6553.5).

**Mitigation**: `validate_sensor_frame()` rejects out-of-range inputs before
encoding. Round-trip decode tests verify correctness.

---

### DEV-009: Rule 8.9 - Object should be at block scope if referenced in one function

**Category**: Advisory
**Suppressed count**: 2
**Locations**: `src/domain/control.c`, `src/domain/engine.c`

**Rationale**: Module-level calibration/config data (`g_calibration`,
`g_physics_config`) is intentionally file-scoped `static` to survive across
function calls within the module.

**Mitigation**: Both objects are `static` and initialised to safe defaults.
Getter/setter functions enforce validation.

---

### DEV-010: Rule 2.5 - Unused macro definitions

**Category**: Advisory
**Suppressed count**: 2
**Locations**: `include/config.h`, `include/hal.h`

**Rationale**: Configuration constants and guard macros are defined for future
use or compile-time documentation.

**Mitigation**: Reviewed periodically.

---

### DEV-011: Rule 2.3 - Unused type declarations

**Category**: Advisory
**Suppressed count**: 2
**Locations**: `include/hal.h`

**Rationale**: Type aliases (`HAL_BusFrame`, `HAL_Frame`) are provided for API
clarity even if only one is currently used downstream.

**Mitigation**: Reviewed periodically.

---

### DEV-012: Rule 2.4 - Unused tag declaration

**Category**: Advisory
**Suppressed count**: 1
**Location**: `include/hal.h`

**Rationale**: The `BusFrame` struct tag is retained for debugger visibility
even though most code uses the typedef.

**Mitigation**: Reviewed periodically.

---

### DEV-013: Rule 21.8 - `abort`, `exit`, `system` and related shall not be used

**Category**: Required
**Suppressed count**: 1
**Location**: `src/reporting/logger.c`

**Rationale**: `getenv("CI")` is a read-only query to detect CI environments
where DEBUG logging would generate excessive output. The return value is only
compared against `NULL` - never dereferenced, stored, or written through.

**Mitigation**: Affects only log verbosity. No safety-critical behaviour depends
on environment variables. Called once during initialisation.

---

### DEV-014: Rule 1.2 - Language extensions should not be used

**Category**: Advisory
**Suppressed count**: Not detected by cppcheck (documented proactively)
**Location**: `include/hal.h`

**Rationale**: `__attribute__((packed))` is essential for the `BusFrame` to
match the 13-byte CAN-like wire format. The `HAL_PACKED` macro provides a
conditional compilation path for compilers that do not support the attribute.

**Mitigation**: `_Static_assert(sizeof(BusFrame) == 13U, ...)` detects ABI
violations at compile time.

---

## Enforcement

```
make analyze-misra    # cppcheck --error-exitcode=1 with MISRA addon
```

- **Non-deviated rules**: Any new violation **fails the build** (exit code 1).
- **Deviated rules**: Suppressed in the Makefile via `--suppress=misra-c2012-X.Y`.
- **Adding a deviation**: Requires a new entry in this document with rationale,
  mitigation, and a corresponding `--suppress` line in the Makefile.
- **CI integration**: The `analyze-misra` target should be included in the CI
  pipeline alongside `analyze-cppcheck` and `analyze-clang-tidy`.
