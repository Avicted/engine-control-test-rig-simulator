# ADR-003: Layered Dependency Direction Enforcement

## Status

Accepted

## Context

Embedded control software frequently suffers from circular dependencies and uncontrolled coupling between application logic, hardware abstraction, and domain rules. In safety-critical systems, modularity is a certification requirement - the domain layer must be independently testable without I/O dependencies.

## Decision

The project enforces a **strict unidirectional dependency hierarchy**:

```
app → domain, platform, scenario, reporting, include
scenario → domain, platform, reporting, include
reporting → domain, platform, include
platform → domain, include
domain → include only
```

**Enforcement mechanism**: `tools/check_layering.sh` uses `grep` to scan `#include` directives in each layer and rejects any include that violates the direction. This script is executed by `make analyze-layering` and is part of the `make ci-check` quality gate.

**Key invariant**: The domain layer (`src/domain/`) includes **only** public headers from `include/`. It never includes platform, scenario, reporting, or application headers.

## Consequences

**Positive:**
- Domain logic (`engine.c`, `control.c`) is pure computation - testable without mocking I/O
- HAL changes don't cascading into safety-critical evaluation logic
- Aligns with IEC 61508 modular decomposition requirements
- Architectural violations are detected automatically in CI

**Negative:**
- Cross-cutting concerns (e.g., logging from domain layer) must be injected via callbacks or deferred
- Currently the domain layer has no logging - acceptable for a simulator, but a production system might need a logging abstraction in `include/`
- The grep-based enforcement is coarse - it checks `#include` patterns, not actual symbol usage

**Trade-off accepted:** The grep-based approach catches the most common violations (wrong includes) without requiring a custom build tool. A production system would use a build system with explicit module visibility (e.g., Bazel, CMake `target_link_libraries(PRIVATE)`).

## Date

2026-02-25
