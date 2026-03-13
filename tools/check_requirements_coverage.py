#!/usr/bin/env python3
import fnmatch
import re
import sys
from pathlib import Path


DEFINE_PATTERN = re.compile(r'^#define\s+(REQ_ENG_[A-Z0-9_]+)\s+"(REQ-ENG-[A-Z0-9-]+)"\s*$')
SECTION_PATTERN = re.compile(r'^##\s+(.*)$')
TABLE_ROW_PATTERN = re.compile(r'^\|\s*(REQ-ENG-[A-Z0-9-]+)\s*\|\s*(.*?)\s*\|$')
CODE_SPAN_PATTERN = re.compile(r'`([^`]+)`')
UNIT_TEST_PATTERN = re.compile(r'\{"([A-Za-z0-9_*.-]+)",\s*[A-Za-z_][A-Za-z0-9_]*\}')
MAKE_TARGET_PATTERN = re.compile(r'^([A-Za-z0-9_.-]+):')
SYMBOL_PATTERN_TEMPLATE = r'\b{symbol}\s*\('


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


def parse_matrix_table_rows(matrix_path: Path) -> dict[str, dict[str, str]]:
    rows_by_section: dict[str, dict[str, str]] = {}
    current_section = ''

    with matrix_path.open('r', encoding='utf-8') as matrix_file:
        for raw_line in matrix_file:
            line = raw_line.rstrip('\n')
            section_match = SECTION_PATTERN.match(line)
            if section_match is not None:
                current_section = section_match.group(1)
                rows_by_section.setdefault(current_section, {})
                continue

            if not current_section:
                continue

            row_match = TABLE_ROW_PATTERN.match(line)
            if row_match is None:
                continue

            rows_by_section.setdefault(current_section, {})[row_match.group(1)] = row_match.group(2)

    return rows_by_section


def parse_registered_unit_tests(repo_root: Path) -> set[str]:
    registered: set[str] = set()

    for test_file in (repo_root / 'tests' / 'unit').glob('*.c'):
        content = test_file.read_text(encoding='utf-8')
        registered.update(UNIT_TEST_PATTERN.findall(content))

    if not registered:
        raise ValueError('No registered unit tests found under tests/unit')

    return registered


def parse_make_targets(makefile_path: Path) -> set[str]:
    targets: set[str] = set()

    with makefile_path.open('r', encoding='utf-8') as makefile:
        for line in makefile:
            match = MAKE_TARGET_PATTERN.match(line)
            if match is not None:
                targets.add(match.group(1))

    if not targets:
        raise ValueError(f'No Makefile targets found: {makefile_path}')

    return targets


def collect_symbol_sources(repo_root: Path) -> list[Path]:
    search_roots = ['include', 'src', 'tests', 'tools']
    symbol_files: list[Path] = []

    for relative_root in search_roots:
        root_path = repo_root / relative_root
        if not root_path.exists():
            continue
        symbol_files.extend(path for path in root_path.rglob('*') if path.suffix in {'.c', '.h', '.py', '.md', '.sh'})

    return symbol_files


def symbol_exists(symbol: str, symbol_files: list[Path]) -> bool:
    symbol_pattern = re.compile(SYMBOL_PATTERN_TEMPLATE.format(symbol=re.escape(symbol)))

    for path in symbol_files:
        content = path.read_text(encoding='utf-8')
        if symbol_pattern.search(content) is not None:
            return True

    return False


def reference_exists(reference: str,
                     repo_root: Path,
                     registered_unit_tests: set[str],
                     make_targets: set[str],
                     symbol_files: list[Path]) -> bool:
    if '*' in reference:
        patterns = [reference]
        return any(
            fnmatch.fnmatchcase(candidate, pattern)
            for pattern in patterns
            for candidate in (registered_unit_tests | make_targets)
        )

    if reference in registered_unit_tests:
        return True

    if reference in make_targets:
        return True

    if reference.endswith('()'):
        return symbol_exists(reference[:-2], symbol_files)

    if '/' in reference or reference.endswith(('.py', '.c', '.h', '.md', '.sh', '.txt', '.json')):
        return (repo_root / reference).exists()

    return False


def validate_test_evidence(rows_by_section: dict[str, dict[str, str]],
                           expected: set[str],
                           repo_root: Path,
                           registered_unit_tests: set[str],
                           make_targets: set[str],
                           symbol_files: list[Path]) -> list[str]:
    errors: list[str] = []
    evidence_rows = rows_by_section.get('Test Evidence Mapping', {})

    for requirement_id in sorted(expected):
        evidence = evidence_rows.get(requirement_id, '')
        references = CODE_SPAN_PATTERN.findall(evidence)

        if not references:
            errors.append(f'Test Evidence Mapping: {requirement_id} has no machine-validated references')
            continue

        for reference in references:
            if not reference_exists(reference, repo_root, registered_unit_tests, make_targets, symbol_files):
                errors.append(f'Test Evidence Mapping: {requirement_id} references missing evidence `{reference}`')

    return errors


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
    makefile_path = repo_root / 'Makefile'

    try:
        registered = parse_registry(registry_path)
        section_ids = parse_matrix_sections(matrix_path)
        rows_by_section = parse_matrix_table_rows(matrix_path)
        registered_unit_tests = parse_registered_unit_tests(repo_root)
        make_targets = parse_make_targets(makefile_path)
        symbol_files = collect_symbol_sources(repo_root)
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
    errors.extend(validate_test_evidence(rows_by_section,
                                         registered,
                                         repo_root,
                                         registered_unit_tests,
                                         make_targets,
                                         symbol_files))

    if errors:
        print('Requirement coverage matrix is incomplete:')
        for error in errors:
            print(f'  {error}')
        return 1

    print(f'requirements coverage validated: {len(registered)} IDs have definitions, runtime/integration evidence, and test evidence')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())