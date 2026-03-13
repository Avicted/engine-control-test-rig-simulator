#!/usr/bin/env python3
import json
import shutil
import subprocess
import sys


def _is_number(value) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def validate_fallback(data_path: str) -> int:
    allowed_top = {"schema_version", "software_version", "build_commit", "scenarios", "summary", "error"}
    allowed_scenario = {"scenario", "requirement_id", "ticks", "expected", "actual", "pass"}
    allowed_tick = {"tick", "rpm", "temp", "oil", "run", "result", "control", "engine_mode"}
    allowed_summary = {"passed", "total"}
    allowed_error = {"code", "module", "function", "tick", "severity", "recoverability"}
    result_enum = {"OK", "WARNING", "SHUTDOWN", "ERROR"}
    mode_enum = {"INIT", "STARTING", "RUNNING", "WARNING", "SHUTDOWN", "UNKNOWN"}
    severity_enum = {"INFO", "WARNING", "ERROR", "FATAL"}
    recoverability_enum = {"RECOVERABLE", "NON_RECOVERABLE"}

    with open(data_path, "r", encoding="utf-8") as data_file:
        data = json.load(data_file)

    if not isinstance(data, dict):
        raise ValueError("Top-level JSON must be an object")
    if not set(data.keys()).issubset(allowed_top):
        raise ValueError("Top-level contains unexpected properties")

    for key in ("schema_version", "software_version", "build_commit", "scenarios", "summary"):
        if key not in data:
            raise ValueError(f"Missing required top-level field: {key}")

    if data["schema_version"] != "1.0.0":
        raise ValueError("schema_version must be 1.0.0")
    if (not isinstance(data["software_version"], str)) or (len(data["software_version"]) == 0):
        raise ValueError("software_version must be a non-empty string")
    if (not isinstance(data["build_commit"], str)) or (len(data["build_commit"]) == 0):
        raise ValueError("build_commit must be a non-empty string")
    if not isinstance(data["scenarios"], list):
        raise ValueError("scenarios must be an array")

    for scenario in data["scenarios"]:
        if not isinstance(scenario, dict):
            raise ValueError("Each scenario must be an object")
        if not set(scenario.keys()).issubset(allowed_scenario):
            raise ValueError("Scenario contains unexpected properties")
        for key in ("scenario", "requirement_id", "ticks", "expected", "actual", "pass"):
            if key not in scenario:
                raise ValueError(f"Scenario missing field: {key}")
        if (not isinstance(scenario["scenario"], str)) or (len(scenario["scenario"]) == 0):
            raise ValueError("scenario must be non-empty string")
        if (not isinstance(scenario["requirement_id"], str)) or (len(scenario["requirement_id"]) == 0):
            raise ValueError("requirement_id must be non-empty string")
        if not isinstance(scenario["ticks"], list):
            raise ValueError("ticks must be an array")
        if scenario["expected"] not in result_enum or scenario["actual"] not in result_enum:
            raise ValueError("expected/actual contains invalid enum")
        if not isinstance(scenario["pass"], bool):
            raise ValueError("pass must be boolean")

        for tick in scenario["ticks"]:
            if not isinstance(tick, dict):
                raise ValueError("tick entry must be object")
            if not set(tick.keys()).issubset(allowed_tick):
                raise ValueError("tick entry contains unexpected properties")
            for key in ("tick", "rpm", "temp", "oil", "run", "result", "control", "engine_mode"):
                if key not in tick:
                    raise ValueError(f"tick entry missing field: {key}")

            if (not isinstance(tick["tick"], int)) or tick["tick"] < 0:
                raise ValueError("tick must be non-negative integer")
            if (not _is_number(tick["rpm"])) or not (0 <= tick["rpm"] <= 10000):
                raise ValueError("rpm out of range")
            if (not _is_number(tick["temp"])) or not (-50 <= tick["temp"] <= 200):
                raise ValueError("temp out of range")
            if (not _is_number(tick["oil"])) or not (0 <= tick["oil"] <= 10):
                raise ValueError("oil out of range")
            if tick["run"] not in (0, 1):
                raise ValueError("run must be 0 or 1")
            if tick["result"] not in result_enum:
                raise ValueError("result enum invalid")
            if (not _is_number(tick["control"])) or not (0 <= tick["control"] <= 100):
                raise ValueError("control out of range")
            if tick["engine_mode"] not in mode_enum:
                raise ValueError("engine_mode enum invalid")

    summary = data["summary"]
    if (not isinstance(summary, dict)) or (not set(summary.keys()).issubset(allowed_summary)):
        raise ValueError("summary must be strict object")
    if ("passed" not in summary) or ("total" not in summary):
        raise ValueError("summary requires passed and total")
    if (not isinstance(summary["passed"], int)) or summary["passed"] < 0:
        raise ValueError("summary.passed must be non-negative integer")
    if (not isinstance(summary["total"], int)) or summary["total"] < 0:
        raise ValueError("summary.total must be non-negative integer")

    if "error" in data:
        error = data["error"]
        if (not isinstance(error, dict)) or (not set(error.keys()).issubset(allowed_error)):
            raise ValueError("error must be strict object")
        for key in ("code", "module", "tick", "severity", "recoverability"):
            if key not in error:
                raise ValueError(f"error missing field: {key}")
        if (not isinstance(error["code"], str)) or (len(error["code"]) == 0):
            raise ValueError("error.code must be non-empty string")
        if (not isinstance(error["module"], str)) or (len(error["module"]) == 0):
            raise ValueError("error.module must be non-empty string")
        if (not isinstance(error["tick"], int)) or error["tick"] < 0:
            raise ValueError("error.tick must be non-negative integer")
        if error["severity"] not in severity_enum:
            raise ValueError("error.severity enum invalid")
        if error["recoverability"] not in recoverability_enum:
            raise ValueError("error.recoverability enum invalid")

    return 0


def validate_with_ajv(data_path: str, schema_path: str) -> int:
    if shutil.which("ajv") is None:
        return 2

    proc = subprocess.run(
        [
            "ajv",
            "validate",
            "--spec=draft2020",
            "-s",
            schema_path,
            "-d",
            data_path,
        ],
        check=False,
    )
    return proc.returncode


def validate_with_jsonschema(data_path: str, schema_path: str) -> int:
    try:
        from jsonschema import Draft202012Validator
    except Exception:
        return 2

    with open(schema_path, "r", encoding="utf-8") as schema_file:
        schema = json.load(schema_file)

    with open(data_path, "r", encoding="utf-8") as data_file:
        data = json.load(data_file)

    Draft202012Validator(schema).validate(data)
    return 0


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: validate_json_contract.py <json-file> <schema-file>")
        return 1

    data_path = sys.argv[1]
    schema_path = sys.argv[2]

    ajv_rc = validate_with_ajv(data_path, schema_path)
    if ajv_rc == 0:
        return 0
    if ajv_rc not in (0, 2):
        return ajv_rc

    try:
        jsonschema_rc = validate_with_jsonschema(data_path, schema_path)
        if jsonschema_rc == 0:
            return 0
        if jsonschema_rc == 2:
            return validate_fallback(data_path)
        return jsonschema_rc
    except Exception as exc:
        print(f"Schema validation failed: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
