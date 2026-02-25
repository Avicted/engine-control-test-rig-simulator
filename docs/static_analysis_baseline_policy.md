# Static Analysis Baseline Policy

This project enforces a zero-warning baseline for safety-oriented validation behavior.

## Required Checks

- `cppcheck` (`--enable=all`) must pass with zero errors.
- `clang-tidy` (`clang-analyzer-*`) must pass with warnings treated as errors.
- AddressSanitizer and UndefinedBehaviorSanitizer runs must complete without runtime diagnostics.
- Layering checks in `tools/check_layering.sh` must pass.

## CI Policy

- `make analyze` is required in CI.
- Any warning or sanitizer finding is treated as a build failure.
- Suppressions require documented rationale and reviewer approval.

## Determinism Constraint

Analysis and sanitizer checks must not introduce nondeterministic behavior into production code paths.
