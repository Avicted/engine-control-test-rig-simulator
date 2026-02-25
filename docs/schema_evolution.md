# Schema Evolution Policy

## Current Versions

| Component          | Version |
| ------------------ | ------- |
| Schema version     | 1.0.0   |
| Software version   | 1.3.0   |

## Versioning Policy

This project follows **Semantic Versioning** (MAJOR.MINOR.PATCH) for the
JSON output schema:

| Change type                            | Bump    |
| -------------------------------------- | ------- |
| Remove or rename a required field      | MAJOR   |
| Change the type of an existing field   | MAJOR   |
| Remove an enum value                   | MAJOR   |
| Add a new required field               | MINOR   |
| Add a new optional field               | MINOR   |
| Add a new enum value                   | MINOR   |
| Fix documentation / no output change   | PATCH   |

### Rules

1. **MAJOR** changes require updating the `schema_version` string and the
   JSON schema file simultaneously.
2. **MINOR** changes must still validate against the previous minor schema.
3. CI enforces that every output contains `schema_version`, `software_version`,
   and `build_commit` in the metadata header.
4. Breaking changes must be documented in `CHANGELOG.md`.

## Change Log

### 1.0.0 (initial)

- `schema_version`, `software_version`, `build_commit` in header.
- `scenario_name`, `requirement_id`, `expected_result` per scenario.
- `ticks[]` array with `tick`, `rpm`, `temperature`, `oil_pressure`,
  `is_running`, `mode`, `control_output`, `evaluation_result`.
- `summary` block with `passed`, `failed`, `total`.
