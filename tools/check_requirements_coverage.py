#!/usr/bin/env python3
import re
import sys
from pathlib import Path


DEFINE_PATTERN = re.compile(r'^#define\s+(REQ_ENG_[A-Z0-9_]+)\s+"(REQ-ENG-[A-Z0-9-]+)"\s*$')
SECTION_PATTERN = re.compile(r'^##\s+(.*)$')


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


def parse_matrix_sections(matrix_path: Path) -> dict[str, set[str]]:
    section_ids: dict[str, set[str]] = {}
    current_section = ''

    with matrix_path.open('r', encoding='utf-8') as matrix_file:
        for raw_line in matrix_file:
            line = raw_line.rstrip('\n')
            section_match = SECTION_PATTERN.match(line)
            if section_match is not None:
                current_section = section_match.group(1)
                section_ids.setdefault(current_section, set())
                continue

            if (not current_section) or (not line.startswith('| REQ-ENG-')):
                continue

            requirement_id = line.split('|')[1].strip()
            section_ids.setdefault(current_section, set()).add(requirement_id)

    return section_ids


def validate_section(section_name: str, section_ids: dict[str, set[str]], expected: set[str]) -> list[str]:
    actual = section_ids.get(section_name, set())
    missing = sorted(expected - actual)
    if not missing:
        return []
    return [f'{section_name}: missing {requirement_id}' for requirement_id in missing]


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    registry_path = repo_root / 'include' / 'requirements.h'
    matrix_path = repo_root / 'docs' / 'requirements_traceability_matrix.md'

    try:
        registered = parse_registry(registry_path)
        section_ids = parse_matrix_sections(matrix_path)
    except Exception as exc:
        print(f'requirements coverage validation failed: {exc}')
        return 1

    errors = []
    definition_ids = section_ids.get('Requirement Definitions', set()) | section_ids.get('Supporting Traceability Definitions', set())
    if definition_ids != registered:
        missing = sorted(registered - definition_ids)
        extra = sorted(definition_ids - registered)
        for requirement_id in missing:
            errors.append(f'Definition sections: missing {requirement_id}')
        for requirement_id in extra:
            errors.append(f'Definition sections: unknown {requirement_id}')

    errors.extend(validate_section('Runtime and Integration Evidence Mapping', section_ids, registered))
    errors.extend(validate_section('Test Evidence Mapping', section_ids, registered))

    if errors:
        print('Requirement coverage matrix is incomplete:')
        for error in errors:
            print(f'  {error}')
        return 1

    print(f'requirements coverage validated: {len(registered)} IDs have definitions, runtime/integration evidence, and test evidence')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())