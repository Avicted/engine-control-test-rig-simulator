# Review Checklist

Use this checklist to run a fast, repeatable technical review.

## 1) Build and Quality Gates

Run:

```bash
make clean
make all
make test-unit
make ci-check
```

Expected:

- Build succeeds with no compiler warnings promoted to errors.
- Unit tests pass.
- `cppcheck`, `clang-tidy`, layering checks, and sanitizer-enabled debug build pass.

## 2) Deterministic Regression Output

Run:

```bash
./build/testrig --run-all --json > /tmp/testrig_review.json
./build/testrig --run-all --json > /tmp/testrig_review_2.json
cmp /tmp/testrig_review.json /tmp/testrig_review_2.json
```

Expected:

- `cmp` reports no differences.
- JSON contains `schema_version`, `software_version`, `requirement_id`, and `summary`.

## 3) Script Parser Strictness

Run malformed-input checks:

```bash
printf 'TICK 1 RPM nan TEMP 80 OIL 3 RUN 1\n' > /tmp/bad_nan.txt
./build/testrig --script /tmp/bad_nan.txt --strict

printf 'TICK 2 RPM 2200 TEMP 80 OIL 3 RUN 1\nTICK 1 RPM 2300 TEMP 81 OIL 3 RUN 1\n' > /tmp/bad_tick_order.txt
./build/testrig --script /tmp/bad_tick_order.txt --strict
```

Expected:

- Non-zero exit code for malformed inputs.
- Clear parse failure message.

## 4) Requirement Traceability

Inspect:

- `docs/requirements_traceability_matrix.md`
- JSON output from `--run-all --json`

Expected:

- Scenario names map to requirement IDs in the matrix.
- JSON `requirement_id` values are present and consistent with matrix intent.

## 5) ABI/Contract Guardrails

Inspect headers:

- `include/engine.h`
- `include/hal.h`
- `include/script_parser.h`
- `include/test_runner.h`

Expected:

- `_Static_assert` checks compile successfully.
- Public APIs document preconditions and postconditions.
