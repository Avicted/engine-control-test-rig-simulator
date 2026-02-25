#!/usr/bin/env sh

set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$repo_root"

violations=0

check_forbidden_includes() {
    layer_path="$1"
    forbidden_regex="$2"
    description="$3"

    if grep -R -n -E --include='*.c' --include='*.h' "#include \"(${forbidden_regex})\"" "$layer_path" >/tmp/layer_check_matches.txt 2>/dev/null; then
        echo "Layering violation: $description"
        cat /tmp/layer_check_matches.txt
        violations=1
    fi
}

check_forbidden_includes "src/domain" "(app/)?test_runner\.h|(scenario/)?script_parser\.h|scenario/scenario_.*\.h|(reporting/)?logger\.h|(reporting/)?output\.h|(reporting/)?version\.h|(platform/)?hal\.h" "src/domain must not depend on app/scenario/reporting/platform headers"
check_forbidden_includes "src/platform" "(app/)?test_runner\.h|(scenario/)?script_parser\.h|scenario/scenario_.*\.h|(reporting/)?logger\.h|(reporting/)?output\.h|(reporting/)?version\.h" "src/platform must not depend on app/scenario/reporting headers"
check_forbidden_includes "src/scenario" "(app/)?test_runner\.h" "src/scenario must not depend on app headers"
check_forbidden_includes "src/reporting" "(app/)?test_runner\.h" "src/reporting must not depend on app headers"

rm -f /tmp/layer_check_matches.txt

if [ "$violations" -ne 0 ]; then
    echo "Layering checks failed"
    exit 1
fi

echo "Layering checks passed"
