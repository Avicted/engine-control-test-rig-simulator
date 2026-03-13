#!/usr/bin/env python3
import re
import sys
from pathlib import Path


REQUIREMENT_PATTERN = re.compile(r'REQ-ENG-[A-Z0-9-]+')
DEFINE_PATTERN = re.compile(r'^#define\s+(REQ_ENG_[A-Z0-9_]+)\s+"(REQ-ENG-[A-Z0-9-]+)"\s*$')


def parse_registry(registry_path: Path) -> set[str]:
    registered: set[str] = set()

    with registry_path.open('r', encoding='utf-8') as registry_file:
        for line in registry_file:
            match = DEFINE_PATTERN.match(line.strip())
            if match is not None:
                registered.add(match.group(2))

    if not registered:
        raise ValueError(f'No requirement IDs found in registry: {registry_path}')

    return registered


def collect_literals(base_dir: Path, relative_paths: list[str]) -> dict[str, set[str]]:
    literals_by_file: dict[str, set[str]] = {}

    for relative_path in relative_paths:
        file_path = base_dir / relative_path
        content = file_path.read_text(encoding='utf-8')
        literals = set(REQUIREMENT_PATTERN.findall(content))
        if literals:
            literals_by_file[relative_path] = literals

    return literals_by_file


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    registry_path = repo_root / 'include' / 'requirements.h'
    scan_paths = [
        'docs/requirements_traceability_matrix.md',
        'include/control.h',
        'include/engine.h',
        'include/hal.h',
        'include/script_parser.h',
        'include/test_runner.h',
        'src/scenario/scenario_report.h',
        'src/scenario/scenario_profiles.h',
        'visualization/scenarios.json',
    ]

    try:
        registered = parse_registry(registry_path)
        literals_by_file = collect_literals(repo_root, scan_paths)
    except Exception as exc:
        print(f'requirements registry validation failed: {exc}')
        return 1

    referenced = set().union(*literals_by_file.values()) if literals_by_file else set()
    missing = sorted(referenced - registered)
    unused = sorted(registered - referenced)

    if missing:
        print('Missing requirement IDs from include/requirements.h:')
        for requirement_id in missing:
            owners = sorted(path for path, values in literals_by_file.items() if requirement_id in values)
            print(f'  {requirement_id}: {", ".join(owners)}')
        return 1

    print(f'requirements registry validated: {len(registered)} IDs registered, {len(referenced)} IDs referenced')
    if unused:
        print('Note: registered but not currently referenced in scoped docs/headers:')
        for requirement_id in unused:
            print(f'  {requirement_id}')

    return 0


if __name__ == '__main__':
    raise SystemExit(main())