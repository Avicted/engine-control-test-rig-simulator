# ADR-004: JSON Schema for Machine-Readable Output

## Status

Accepted

## Context

The simulator produces test reports that must be consumed by:

1. **CI pipelines** - for automated pass/fail gating
2. **Visualization tools** - for human review of scenario results
3. **Traceability systems** - for mapping test outcomes to requirements

Plain-text output is human-readable but fragile for machine parsing. A structured format with a formal schema enables reliable downstream processing.

## Decision

The simulator emits **JSON output** validated against a **JSON Schema Draft 2020-12** definition:

- Schema: `schema/engine_test_rig.schema.json`
- Validation chain: `ajv` (Node.js) → `jsonschema` (Python) → hand-rolled grep fallback
- CI gate: `make validate-json` and `make validate-json-contract`

The JSON envelope includes:

```json
{
  "schema_version": "1.0.0",
  "software_version": "...",
  "build_commit": "...",
  "results": [
    {
      "scenario": "...",
      "requirement_id": "REQ-ENG-001",
      "expected": "ENGINE_SHUTDOWN",
      "actual": "ENGINE_SHUTDOWN",
      "pass": true,
      "ticks": [...]
    }
  ],
  "summary": { "passed": N, "failed": M, "total": N+M }
}
```

## Consequences

**Positive:**
- **Machine-parseable**: CI can `jq .summary.failed` to gate builds
- **Schema-validated**: structural regressions detected before tests run
- **Version-stamped**: `schema_version` + `build_commit` enable reproducibility audits
- **Requirement-linked**: each scenario carries its `requirement_id` for traceability
- **Visualization-ready**: the Raylib dashboard reads JSON directly - no parsing coupling

**Negative:**
- Hand-rolled JSON emitter in `scenario_report.c` - no string escaping, fragile
- JSON Schema Draft 2020-12 requires modern validators (ajv ≥8, jsonschema ≥4)
- Schema evolution requires careful versioning (breaking changes need major version bump)
- Three-tier validation chain adds CI complexity (Node.js, Python, grep)

**Trade-off accepted:** The hand-rolled emitter is sufficient for the simulator's controlled output vocabulary (no user-supplied strings in JSON fields). A production system would use a vetted JSON library (cJSON, jansson).

## Date

2026-02-25
