#!/usr/bin/env python3

import argparse
import json
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run a black-box audit of the shipped release bundle using the packaged simulator inputs."
    )
    parser.add_argument(
        "--bundle-dir",
        help="Path to the unpacked release bundle root. Defaults to the parent of this script when run from tools/.",
    )
    parser.add_argument(
        "--command-prefix",
        default="",
        help="Optional command prefix used to launch the packaged binaries, for example 'wine'.",
    )
    parser.add_argument(
        "--visualizer-timeout",
        type=float,
        default=3.0,
        help="Seconds to allow the visualizer to run before treating the timeout as a successful smoke test.",
    )
    parser.add_argument(
        "--skip-visualizer",
        action="store_true",
        help="Skip the visualizer smoke test and only audit the simulator outputs.",
    )
    parser.add_argument(
        "--skip-visualization-regeneration",
        action="store_true",
        help="Skip rebuilding visualization/scenarios.json from the packaged simulator inputs.",
    )
    return parser.parse_args()


def determine_bundle_dir(raw_bundle_dir: str | None) -> Path:
    if raw_bundle_dir:
        return Path(raw_bundle_dir).resolve()

    script_path = Path(__file__).resolve()
    if script_path.parent.name == "tools":
        return script_path.parent.parent
    return Path.cwd().resolve()


def parse_prefix(raw_prefix: str) -> list[str]:
    return shlex.split(raw_prefix)


def find_simulator_command(bundle_dir: Path, prefix: list[str]) -> list[str]:
    launcher_path = bundle_dir / "run-testrig.sh"
    if launcher_path.is_file() and not prefix:
        return [str(launcher_path)]

    for candidate in ("testrig.exe", "testrig"):
        binary_path = bundle_dir / candidate
        if binary_path.is_file():
            return [*prefix, str(binary_path)]

    raise FileNotFoundError("could not find packaged simulator executable")


def find_visualizer_command(bundle_dir: Path, prefix: list[str]) -> list[str]:
    launcher_path = bundle_dir / "run-visualizer.sh"
    if launcher_path.is_file() and not prefix:
        return [str(launcher_path)]

    for candidate in ("visualizer.exe", "visualizer"):
        binary_path = bundle_dir / candidate
        if binary_path.is_file():
            return [*prefix, str(binary_path)]

    raise FileNotFoundError("could not find packaged visualizer executable")


def run_command(command: list[str], cwd: Path, timeout: float | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=cwd,
        check=False,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def require_success(completed: subprocess.CompletedProcess[str], description: str) -> None:
    if completed.returncode == 0:
        return
    raise RuntimeError(
        f"{description} failed with exit code {completed.returncode}\n"
        f"stdout:\n{completed.stdout}\n"
        f"stderr:\n{completed.stderr}"
    )


def load_json_or_fail(raw_json: str, description: str) -> dict:
    try:
        payload = json.loads(raw_json)
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"{description} did not produce valid JSON: {exc}\noutput:\n{raw_json}") from exc
    if not isinstance(payload, dict):
        raise RuntimeError(f"{description} JSON payload must be an object")
    return payload


def validate_with_schema(bundle_dir: Path, output_path: Path) -> None:
    validator = bundle_dir / "tools" / "validate_json_contract.py"
    schema = bundle_dir / "schema" / "engine_test_rig.schema.json"
    if not validator.is_file():
        raise FileNotFoundError(f"validator script not found: {validator}")
    if not schema.is_file():
        raise FileNotFoundError(f"schema not found: {schema}")

    completed = run_command([sys.executable, str(validator), str(output_path), str(schema)], cwd=bundle_dir)
    require_success(completed, f"schema validation for {output_path.name}")


def validate_shipped_visualization_bundle(bundle_dir: Path, shipped_visualization_bundle: Path) -> dict:
    validate_with_schema(bundle_dir, shipped_visualization_bundle)
    payload = load_json_or_fail(
        shipped_visualization_bundle.read_text(encoding="utf-8"),
        "packaged visualization bundle",
    )
    summary = payload.get("summary")
    scenarios = payload.get("scenarios")
    if not isinstance(summary, dict):
        raise RuntimeError("packaged visualization bundle is missing a valid summary object")
    if not isinstance(scenarios, list):
        raise RuntimeError("packaged visualization bundle is missing a valid scenarios array")
    if summary.get("passed") != summary.get("total"):
        raise RuntimeError("packaged visualization bundle does not report all scenarios passing")
    if summary.get("total") != len(scenarios):
        raise RuntimeError("packaged visualization bundle summary does not match the scenario count")
    return payload


def run_json_probe(bundle_dir: Path,
                   simulator_command: list[str],
                   args: list[str],
                   output_path: Path,
                   description: str) -> dict:
    completed = run_command([*simulator_command, *args], cwd=bundle_dir)
    require_success(completed, description)
    output_path.write_text(completed.stdout, encoding="utf-8")
    validate_with_schema(bundle_dir, output_path)
    return load_json_or_fail(completed.stdout, description)


def run_invalid_script_probe(bundle_dir: Path, simulator_command: list[str], output_path: Path) -> None:
    invalid_script = bundle_dir / "tests" / "integration" / "invalid_script.txt"
    if not invalid_script.is_file():
        raise FileNotFoundError(f"invalid-script fixture not found: {invalid_script}")

    completed = run_command([*simulator_command, "--script", str(invalid_script), "--json"], cwd=bundle_dir)
    if completed.returncode == 0:
        raise RuntimeError("invalid scripted scenario unexpectedly passed")

    output_path.write_text(completed.stdout, encoding="utf-8")
    validate_with_schema(bundle_dir, output_path)
    payload = load_json_or_fail(completed.stdout, "invalid-script probe")
    error = payload.get("error")
    if not isinstance(error, dict):
        raise RuntimeError("invalid-script probe did not return an error object")
    if error.get("code") != "STATUS_PARSE_ERROR":
        raise RuntimeError(f"invalid-script probe returned unexpected error code: {error.get('code')}")
    if error.get("module") != "script_parser":
        raise RuntimeError(f"invalid-script probe returned unexpected module: {error.get('module')}")
    if error.get("severity") != "ERROR":
        raise RuntimeError(f"invalid-script probe returned unexpected severity: {error.get('severity')}")


def regenerate_visualization_payload(bundle_dir: Path, simulator_command: list[str]) -> dict:
    scenarios_dir = bundle_dir / "scenarios"
    script_paths = sorted(scenarios_dir.glob("*.txt"))
    if not script_paths:
        raise FileNotFoundError(f"no scenario scripts found in: {scenarios_dir}")

    combined: dict | None = None
    combined_scenarios = []

    for script_path in script_paths:
        completed = run_command([*simulator_command, "--script", str(script_path), "--json"], cwd=bundle_dir)
        require_success(completed, f"scenario replay for {script_path.name}")
        payload = load_json_or_fail(completed.stdout, f"scenario replay for {script_path.name}")
        scenarios = payload.get("scenarios")
        if not isinstance(scenarios, list) or len(scenarios) != 1:
            raise RuntimeError(f"unexpected scenario payload for {script_path.name}")

        scenario_payload = dict(scenarios[0])
        scenario_payload["scenario"] = script_path.stem

        if combined is None:
            combined = {
                "schema_version": payload.get("schema_version", "1.0.0"),
                "software_version": payload.get("software_version", "unknown"),
                "build_commit": payload.get("build_commit", "unknown"),
            }

        combined_scenarios.append(scenario_payload)

    assert combined is not None
    combined["scenarios"] = combined_scenarios
    combined["summary"] = {
        "passed": sum(1 for scenario in combined_scenarios if scenario.get("pass") is True),
        "total": len(combined_scenarios),
    }
    return combined


def smoke_test_visualizer(bundle_dir: Path, visualizer_command: list[str], timeout_seconds: float) -> None:
    scenario_bundle = bundle_dir / "visualization" / "scenarios.json"
    if not scenario_bundle.is_file():
        raise FileNotFoundError(f"visualization bundle not found: {scenario_bundle}")

    try:
        completed = run_command([*visualizer_command, str(scenario_bundle)], cwd=bundle_dir, timeout=timeout_seconds)
    except subprocess.TimeoutExpired:
        return

    if completed.returncode != 0:
        raise RuntimeError(
            f"visualizer smoke test failed with exit code {completed.returncode}\n"
            f"stdout:\n{completed.stdout}\n"
            f"stderr:\n{completed.stderr}"
        )


def main() -> int:
    args = parse_args()
    bundle_dir = determine_bundle_dir(args.bundle_dir)
    prefix = parse_prefix(args.command_prefix)

    simulator_command = find_simulator_command(bundle_dir, prefix)
    visualizer_command = find_visualizer_command(bundle_dir, prefix)

    shipped_visualization_bundle = bundle_dir / "visualization" / "scenarios.json"
    if not shipped_visualization_bundle.is_file():
        raise FileNotFoundError(f"packaged visualization bundle not found: {shipped_visualization_bundle}")
    shipped_payload = validate_shipped_visualization_bundle(bundle_dir, shipped_visualization_bundle)

    with tempfile.TemporaryDirectory(prefix="release-audit-") as temp_dir:
        temp_path = Path(temp_dir)

        console_probe = run_command([*simulator_command, "--run-all"], cwd=bundle_dir)
        require_success(console_probe, "simulator console test suite")

        run_all_payload = run_json_probe(
            bundle_dir,
            simulator_command,
            ["--run-all", "--json"],
            temp_path / "run-all.json",
            "simulator JSON test suite",
        )
        summary = run_all_payload.get("summary")
        if not isinstance(summary, dict) or summary.get("passed") != summary.get("total"):
            raise RuntimeError("simulator JSON test suite did not report all scenarios passing")

        run_json_probe(
            bundle_dir,
            simulator_command,
            ["--run-all", "--config", "calibration.json", "--json"],
            temp_path / "run-all-config.json",
            "runtime calibration JSON suite",
        )
        run_json_probe(
            bundle_dir,
            simulator_command,
            ["--script", "scenarios/normal_operation.txt", "--json"],
            temp_path / "normal-operation.json",
            "normal operation JSON scenario",
        )
        run_invalid_script_probe(bundle_dir, simulator_command, temp_path / "invalid-script.json")

        if not args.skip_visualization_regeneration:
            regenerated_payload = regenerate_visualization_payload(bundle_dir, simulator_command)
            regenerated_path = temp_path / "regenerated-visualization.json"
            regenerated_path.write_text(json.dumps(regenerated_payload, indent=2) + "\n", encoding="utf-8")
            validate_with_schema(bundle_dir, regenerated_path)

            if regenerated_payload != shipped_payload:
                raise RuntimeError("packaged visualization/scenarios.json does not match the simulator outputs in the bundle")

        if not args.skip_visualizer:
            smoke_test_visualizer(bundle_dir, visualizer_command, args.visualizer_timeout)

    print("release audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())