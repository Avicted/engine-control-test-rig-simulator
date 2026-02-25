#!/usr/bin/env sh
# -------------------------------------------------------------------
# Deterministic Replay Test (Section 2.2)
#
# Runs the simulator twice on the same input and compares SHA-256
# hashes of the JSON output.  Any mismatch is a determinism violation.
# -------------------------------------------------------------------

set -eu

TESTRIG="${1:-./build/testrig}"
TMP1="$(mktemp)"
TMP2="$(mktemp)"

cleanup() {
    rm -f "$TMP1" "$TMP2"
}
trap cleanup EXIT

echo "=== Deterministic Replay Check ==="

# Run 1
"$TESTRIG" --run-all --json > "$TMP1" 2>/dev/null

# Run 2
"$TESTRIG" --run-all --json > "$TMP2" 2>/dev/null

HASH1="$(sha256sum "$TMP1" | awk '{print $1}')"
HASH2="$(sha256sum "$TMP2" | awk '{print $1}')"

echo "Run 1 hash: $HASH1"
echo "Run 2 hash: $HASH2"

if [ "$HASH1" != "$HASH2" ]; then
    echo "FAIL: Determinism violation - output differs between identical runs"
    diff "$TMP1" "$TMP2" | head -20
    exit 1
fi

echo "PASS: Identical input produces identical output (SHA-256 match)"
exit 0
