#!/usr/bin/env python3

import argparse
import os
import shutil
import stat
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a release archive containing the simulator, visualizer, and runtime data."
    )
    parser.add_argument("--platform", required=True, help="Platform label to embed in the bundle name.")
    parser.add_argument("--testrig", required=True, help="Path to the simulator executable.")
    parser.add_argument("--visualizer", required=True, help="Path to the visualizer executable.")
    parser.add_argument("--archive", required=True, help="Output archive path (.zip or .tar.gz).")
    parser.add_argument(
        "--extra-entry",
        action="append",
        default=[],
        help="Extra runtime file to place in the bundle. Format: source[:relative/destination].",
    )
    parser.add_argument(
        "--extra-tree",
        action="append",
        default=[],
        help="Extra directory tree to place in the bundle. Format: source_dir[:relative/destination].",
    )
    parser.add_argument(
        "--linux-launchers",
        action="store_true",
        help="Add Linux launcher scripts that set LD_LIBRARY_PATH to the bundled lib directory.",
    )
    return parser.parse_args()


def ensure_file(path: Path, label: str) -> Path:
    if not path.is_file():
        raise FileNotFoundError(f"{label} not found: {path}")
    return path


def copy_file(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def copy_tree(source: Path, destination: Path) -> None:
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(source, destination)


def parse_mapping(raw_value: str) -> tuple[Path, Path | None]:
    source_part, separator, destination_part = raw_value.partition(":")
    source_path = Path(source_part).expanduser().resolve()
    if not separator:
        return source_path, None
    return source_path, Path(destination_part)


def write_linux_launchers(bundle_dir: Path, testrig_name: str, visualizer_name: str) -> None:
    launchers = {
        "run-testrig.sh": f"#!/usr/bin/env sh\nset -eu\nSCRIPT_DIR=$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)\nexport LD_LIBRARY_PATH=\"$SCRIPT_DIR/lib${{LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}}\"\nexec \"$SCRIPT_DIR/{testrig_name}\" \"$@\"\n",
        "run-visualizer.sh": f"#!/usr/bin/env sh\nset -eu\nSCRIPT_DIR=$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)\nexport LD_LIBRARY_PATH=\"$SCRIPT_DIR/lib${{LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}}\"\nexec \"$SCRIPT_DIR/{visualizer_name}\" \"$@\"\n",
    }

    for launcher_name, content in launchers.items():
        launcher_path = bundle_dir / launcher_name
        launcher_path.write_text(content, encoding="utf-8")
        launcher_path.chmod(launcher_path.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def write_run_notes(destination: Path,
                    bundle_name: str,
                    testrig_name: str,
                    visualizer_name: str,
                    has_linux_launchers: bool) -> None:
    is_windows_bundle = testrig_name.endswith(".exe")
    testrig_cmd = f"./{testrig_name} --run-all"
    testrig_version_cmd = f"./{testrig_name} --version"
    testrig_script_cmd = f"./{testrig_name} --script scenarios/normal_operation.txt --json"
    visualizer_cmd = f"./{visualizer_name} visualization/scenarios.json"
    audit_cmd = "python3 tools/release_audit.py"

    if has_linux_launchers:
        testrig_version_cmd = "./run-testrig.sh --version"
        testrig_cmd = "./run-testrig.sh --run-all"
        testrig_script_cmd = "./run-testrig.sh --script scenarios/normal_operation.txt --json"
        visualizer_cmd = "./run-visualizer.sh visualization/scenarios.json"

    if is_windows_bundle:
        testrig_version_cmd = f".\\{testrig_name} --version"
        testrig_cmd = f".\\{testrig_name} --run-all"
        testrig_script_cmd = f".\\{testrig_name} --script scenarios\\normal_operation.txt --json"
        visualizer_cmd = f".\\{visualizer_name} visualization\\scenarios.json"
        audit_cmd = "py -3 tools\\release_audit.py"

    linux_review_cmds = [
        "./run-testrig.sh --version" if has_linux_launchers else f"./{testrig_name} --version",
        "./run-testrig.sh --run-all" if has_linux_launchers else f"./{testrig_name} --run-all",
        "./run-testrig.sh --script scenarios/normal_operation.txt --json" if has_linux_launchers else f"./{testrig_name} --script scenarios/normal_operation.txt --json",
        "./run-visualizer.sh visualization/scenarios.json" if has_linux_launchers else f"./{visualizer_name} visualization/scenarios.json",
    ]
    windows_review_cmds = [
        f".\\{testrig_name} --version",
        f".\\{testrig_name} --run-all",
        f".\\{testrig_name} --script scenarios\\normal_operation.txt --json",
        f".\\{visualizer_name} visualization\\scenarios.json",
    ]
    wine_review_cmds = [
        "wine ./testrig.exe --version",
        "wine ./testrig.exe --run-all",
        "wine ./testrig.exe --script scenarios/normal_operation.txt --json",
        "wine ./visualizer.exe visualization/scenarios.json",
    ]

    lines = [
        f"{bundle_name}",
        "",
        "Contents:",
        f"- {testrig_name}: simulator CLI",
        f"- {visualizer_name}: Raylib scenario visualizer",
        "- calibration.json: optional runtime calibration input",
        "- scenarios/: scripted scenario files for the simulator",
        "- schema/engine_test_rig.schema.json: JSON contract used by the shipped audit suite",
        "- tests/integration/invalid_script.txt: negative test fixture for parser error validation",
        "- tools/release_audit.py: portable black-box release audit for the packaged simulator",
        "- visualization/scenarios.json: pre-generated visualizer scenario bundle",
        "- visualization/PxPlus_IBM_EGA_8x14.ttf: visualizer font asset",
        "",
        "Quick start:",
        f"- Show the simulator version: {testrig_version_cmd}",
        f"- Run the packaged validation sweep: {testrig_cmd}",
        f"- Run a scripted scenario and emit JSON: {testrig_script_cmd}",
        f"- Start the visualizer with the shipped scenario bundle: {visualizer_cmd}",
        f"- Run the shipped audit suite: {audit_cmd}",
        "",
        "Reviewer commands:",
        "Notes:",
        "- Running the simulator or visualizer with no arguments prints usage; pass one of the commands above.",
        "- The visualizer always needs at least one scenarios.json path, and the shipped bundle is visualization/scenarios.json.",
        "- The visualizer loads visualization/PxPlus_IBM_EGA_8x14.ttf via a relative path, so keep the shipped directory layout intact.",
    ]
    if is_windows_bundle:
        lines.extend([
            "- Windows Command Prompt or PowerShell:",
            *(f"  {command}" for command in windows_review_cmds),
            "- Linux with Wine:",
            *(f"  {command}" for command in wine_review_cmds),
        ])
    else:
        lines.extend([
            "- Linux shell:",
            *(f"  {command}" for command in linux_review_cmds),
        ])
    if has_linux_launchers:
        lines.append("- Linux launchers set LD_LIBRARY_PATH so the bundled shared libraries are used first.")
    if is_windows_bundle:
        lines.extend([
            "- This Win64 bundle does not ship run-*.sh wrappers.",
            f"- On Windows, use {testrig_cmd} and {visualizer_cmd} from Command Prompt or PowerShell.",
            "- On Linux, launch the Win64 binaries with Wine: wine ./testrig.exe --run-all and wine ./visualizer.exe visualization/scenarios.json.",
            "- On Linux, run the shipped audit with: python3 tools/release_audit.py --bundle-dir . --command-prefix wine --skip-visualizer --skip-visualization-regeneration",
        ])
    else:
        lines.append("- On Linux, the run-*.sh wrappers are the supported entry points for the packaged binaries.")
        lines.append("- On Windows, use py -3 tools\\release_audit.py or python tools\\release_audit.py to run the audit suite.")
    destination.write_text("\n".join(lines) + "\n", encoding="utf-8")


def create_archive(bundle_dir: Path, archive_path: Path) -> None:
    archive_path.parent.mkdir(parents=True, exist_ok=True)

    if archive_path.suffix == ".zip":
        with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(bundle_dir.rglob("*")):
                archive.write(path, arcname=path.relative_to(bundle_dir.parent))
        return

    if archive_path.name.endswith(".tar.gz"):
        with tarfile.open(archive_path, "w:gz") as archive:
            archive.add(bundle_dir, arcname=bundle_dir.name)
        return

    raise ValueError(f"unsupported archive format: {archive_path}")


def main() -> int:
    args = parse_args()
    repo_root = Path.cwd()
    testrig_path = ensure_file(Path(args.testrig).resolve(), "testrig executable")
    visualizer_path = ensure_file(Path(args.visualizer).resolve(), "visualizer executable")
    archive_path = Path(args.archive).resolve()
    bundle_name = f"engine-control-test-rig-simulator-{args.platform}"

    static_files = [
        ensure_file(repo_root / "README.md", "README"),
        ensure_file(repo_root / "calibration.json", "calibration.json"),
        ensure_file(repo_root / "tools/release_audit.py", "release audit script"),
        ensure_file(repo_root / "tools/validate_json_contract.py", "JSON validator script"),
        ensure_file(repo_root / "visualization/scenarios.json", "visualization scenarios"),
        ensure_file(repo_root / "visualization/PxPlus_IBM_EGA_8x14.ttf", "visualizer font"),
        ensure_file(repo_root / "tests/integration/invalid_script.txt", "invalid-script test fixture"),
    ]
    scenarios_dir = repo_root / "scenarios"
    if not scenarios_dir.is_dir():
        raise FileNotFoundError(f"scenarios directory not found: {scenarios_dir}")
    schema_dir = repo_root / "schema"
    if not schema_dir.is_dir():
        raise FileNotFoundError(f"schema directory not found: {schema_dir}")

    try:
        with tempfile.TemporaryDirectory(prefix="release-bundle-") as tmp_dir:
            bundle_dir = Path(tmp_dir) / bundle_name
            bundle_dir.mkdir(parents=True, exist_ok=True)

            copy_file(testrig_path, bundle_dir / testrig_path.name)
            copy_file(visualizer_path, bundle_dir / visualizer_path.name)

            for file_path in static_files:
                relative_path = file_path.relative_to(repo_root)
                copy_file(file_path, bundle_dir / relative_path)

            copy_tree(scenarios_dir, bundle_dir / "scenarios")
            copy_tree(schema_dir, bundle_dir / "schema")

            for extra_entry in args.extra_entry:
                extra_path, destination_rel = parse_mapping(extra_entry)
                ensure_file(extra_path, "extra runtime file")
                if destination_rel is None:
                    copy_file(extra_path, bundle_dir / extra_path.name)
                else:
                    copy_file(extra_path, bundle_dir / destination_rel)

            for extra_tree in args.extra_tree:
                tree_path, destination_rel = parse_mapping(extra_tree)
                if not tree_path.is_dir():
                    raise FileNotFoundError(f"extra runtime directory not found: {tree_path}")
                if destination_rel is None:
                    copy_tree(tree_path, bundle_dir / tree_path.name)
                elif str(destination_rel) == ".":
                    for child in sorted(tree_path.iterdir()):
                        target = bundle_dir / child.name
                        if child.is_dir():
                            copy_tree(child, target)
                        else:
                            copy_file(child, target)
                else:
                    copy_tree(tree_path, bundle_dir / destination_rel)

            if args.linux_launchers:
                write_linux_launchers(bundle_dir, testrig_path.name, visualizer_path.name)

            write_run_notes(bundle_dir / "RUNNING.txt",
                            bundle_name,
                            testrig_path.name,
                            visualizer_path.name,
                            args.linux_launchers)
            create_archive(bundle_dir, archive_path)
    except (FileNotFoundError, OSError, ValueError) as exc:
        print(str(exc), file=sys.stderr)
        return 1

    print(f"created release archive: {archive_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())