#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate visualization/scenario.json from all scripted scenario .txt files."
    )
    parser.add_argument(
        "--testrig",
        default="./build/testrig",
        help="Path to the test rig executable (default: ./build/testrig)",
    )
    parser.add_argument(
        "--scenarios-dir",
        default="./scenarios",
        help="Directory containing scripted scenario .txt files (default: ./scenarios)",
    )
    parser.add_argument(
        "--output",
        default="./visualization/scenario.json",
        help="Output JSON path (default: ./visualization/scenario.json)",
    )
    return parser.parse_args()


def run_scripted_scenario(testrig: Path, script_path: Path) -> dict:
    completed = subprocess.run(
        [str(testrig), "--script", str(script_path), "--json"],
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(completed.stdout)


def main() -> int:
    args = parse_args()
    repo_root = Path.cwd()
    testrig = (repo_root / args.testrig).resolve() if not Path(args.testrig).is_absolute() else Path(args.testrig)
    scenarios_dir = (repo_root / args.scenarios_dir).resolve() if not Path(args.scenarios_dir).is_absolute() else Path(args.scenarios_dir)
    output_path = (repo_root / args.output).resolve() if not Path(args.output).is_absolute() else Path(args.output)

    if not testrig.is_file():
        print(f"testrig executable not found: {testrig}", file=sys.stderr)
        return 1

    if not scenarios_dir.is_dir():
        print(f"scenarios directory not found: {scenarios_dir}", file=sys.stderr)
        return 1

    script_paths = sorted(scenarios_dir.glob("*.txt"))
    if not script_paths:
        print(f"no .txt scenario files found in: {scenarios_dir}", file=sys.stderr)
        return 1

    combined = None
    combined_scenarios = []

    for script_path in script_paths:
        payload = run_scripted_scenario(testrig, script_path)
        scenarios = payload.get("scenarios")
        if not isinstance(scenarios, list) or len(scenarios) != 1:
            print(f"unexpected JSON payload for script: {script_path}", file=sys.stderr)
            return 1

        scenario_object = dict(scenarios[0])
        scenario_object["scenario"] = script_path.stem

        if combined is None:
            combined = {
                "schema_version": payload.get("schema_version", "1.0.0"),
                "software_version": payload.get("software_version", "unknown"),
                "build_commit": payload.get("build_commit", "unknown"),
            }

        combined_scenarios.append(scenario_object)

    assert combined is not None
    combined["scenarios"] = combined_scenarios
    combined["summary"] = {
        "passed": sum(1 for scenario in combined_scenarios if scenario.get("pass") is True),
        "total": len(combined_scenarios),
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(combined, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())