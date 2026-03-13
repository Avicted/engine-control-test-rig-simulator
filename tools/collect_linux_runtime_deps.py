#!/usr/bin/env python3

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


SKIP_PREFIXES = (
    "linux-vdso",
    "linux-gate",
)

SKIP_NAMES = (
    "ld-linux",
    "ld-musl",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect Linux shared library dependencies for one or more executables."
    )
    parser.add_argument("--output-dir", required=True, help="Directory where copied shared libraries will be placed.")
    parser.add_argument("--binary", action="append", required=True, help="Executable to inspect with ldd.")
    return parser.parse_args()


def should_skip(name: str, resolved_path: str) -> bool:
    if any(name.startswith(prefix) for prefix in SKIP_PREFIXES):
        return True
    if any(part in name for part in SKIP_NAMES):
        return True
    if any(part in resolved_path for part in SKIP_NAMES):
        return True
    return False


def collect_dependencies(binary_path: Path) -> set[Path]:
    result = subprocess.run(
        ["ldd", str(binary_path)],
        check=True,
        capture_output=True,
        text=True,
    )

    dependencies: set[Path] = set()
    for raw_line in result.stdout.splitlines():
        line = raw_line.strip()
        if not line:
            continue

        if "=>" in line:
            left, right = [part.strip() for part in line.split("=>", 1)]
            if right == "not found":
                raise FileNotFoundError(f"missing dependency for {binary_path}: {left}")
            resolved_path = right.split("(", 1)[0].strip()
            if not resolved_path.startswith("/"):
                continue
            if should_skip(left, resolved_path):
                continue
            dependencies.add(Path(resolved_path))
            continue

        token = line.split("(", 1)[0].strip()
        if token.startswith("/") and not should_skip(token, token):
            dependencies.add(Path(token))

    return dependencies


def main() -> int:
    args = parse_args()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    dependencies: set[Path] = set()
    try:
        for binary_arg in args.binary:
            binary_path = Path(binary_arg).resolve()
            if not binary_path.is_file():
                raise FileNotFoundError(f"binary not found: {binary_path}")
            dependencies.update(collect_dependencies(binary_path))
    except (FileNotFoundError, subprocess.CalledProcessError) as exc:
        print(str(exc), file=sys.stderr)
        return 1

    for dependency in sorted(dependencies):
        shutil.copy2(dependency, output_dir / dependency.name)

    for dependency in sorted(dependencies):
        print(dependency)
    return 0


if __name__ == "__main__":
    sys.exit(main())