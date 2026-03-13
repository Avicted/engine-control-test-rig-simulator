#!/usr/bin/env python3

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Collect MinGW runtime DLLs required by one or more Windows executables."
    )
    parser.add_argument("--output-dir", required=True, help="Directory where copied DLLs will be placed.")
    parser.add_argument("--binary", action="append", required=True, help="Windows executable to inspect.")
    parser.add_argument("--search-dir", action="append", required=True, help="Directory to search for dependent DLLs.")
    parser.add_argument(
        "--objdump",
        default="x86_64-w64-mingw32-objdump",
        help="objdump executable to use for dependency inspection.",
    )
    return parser.parse_args()


def is_windows_system_dll(dll_name: str) -> bool:
    upper_name = dll_name.upper()
    if upper_name.startswith("API-MS-WIN-") or upper_name.startswith("EXT-MS-"):
        return True
    return upper_name in {
        "ADVAPI32.DLL",
        "COMDLG32.DLL",
        "GDI32.DLL",
        "KERNEL32.DLL",
        "MSVCRT.DLL",
        "OLE32.DLL",
        "SHELL32.DLL",
        "USER32.DLL",
        "WINMM.DLL",
        "WS2_32.DLL",
    }


def parse_dependencies(binary_path: Path, objdump_executable: str) -> set[str]:
    result = subprocess.run(
        [objdump_executable, "-p", str(binary_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    dependencies: set[str] = set()
    for line in result.stdout.splitlines():
        line = line.strip()
        if not line.startswith("DLL Name:"):
            continue
        dll_name = line.split(":", 1)[1].strip()
        if is_windows_system_dll(dll_name):
            continue
        dependencies.add(dll_name)
    return dependencies


def locate_dll(dll_name: str, search_dirs: list[Path]) -> Path:
    lower_name = dll_name.lower()
    for search_dir in search_dirs:
        candidate = search_dir / dll_name
        if candidate.is_file():
            return candidate
        for child in search_dir.iterdir():
            if child.is_file() and child.name.lower() == lower_name:
                return child
    raise FileNotFoundError(f"unable to locate runtime DLL: {dll_name}")


def main() -> int:
    args = parse_args()
    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    search_dirs = [Path(path).resolve() for path in args.search_dir]

    try:
        dependencies: set[str] = set()
        for binary_arg in args.binary:
            binary_path = Path(binary_arg).resolve()
            if not binary_path.is_file():
                raise FileNotFoundError(f"binary not found: {binary_path}")
            dependencies.update(parse_dependencies(binary_path, args.objdump))

        for dll_name in sorted(dependencies):
            dll_path = locate_dll(dll_name, search_dirs)
            shutil.copy2(dll_path, output_dir / dll_path.name)
            print(dll_path)
    except (FileNotFoundError, OSError, subprocess.CalledProcessError) as exc:
        print(str(exc), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())