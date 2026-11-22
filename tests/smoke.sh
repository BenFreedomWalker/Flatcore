#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "\${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="\${1:-"$ROOT_DIR/build"}"
BIN="$BUILD_DIR/flatcore"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

"$BIN" true
if "$BIN" false; then
    printf 'false returned success unexpectedly\n' >&2
    exit 1
fi

expected='Flatcore smoke test'
actual="$("$BIN" echo "$expected")"
[[ "$actual" == "$expected" ]]

(
    cd "$TMP_DIR"
    "$BIN" touch sample.txt
    "$BIN" echo 'assembly userspace' > input.txt
    "$BIN" cp input.txt copied.txt
    cmp input.txt copied.txt
    "$BIN" mv copied.txt moved.txt
    cmp input.txt moved.txt
    "$BIN" mkdir sample-dir
    "$BIN" rmdir sample-dir
    "$BIN" rm sample.txt input.txt moved.txt
)

printf 'Flatcore smoke tests passed.\n'
