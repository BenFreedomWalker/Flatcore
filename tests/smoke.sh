#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-"$ROOT_DIR/build"}"
case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$ROOT_DIR/$BUILD_DIR" ;;
esac
BIN="$BUILD_DIR/flatcore"
BIN_LINKS="$BUILD_DIR/bin"
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

actual="$("$BIN" basename '/tmp/flatcore-test/')"
[[ "$actual" == 'flatcore-test' ]]

actual="$("$BIN" dirname '/tmp/flatcore-test/file.txt')"
[[ "$actual" == '/tmp/flatcore-test' ]]

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

    "$BIN" ln -s input.txt input-link
    [[ "$("$BIN" readlink input-link)" == 'input.txt' ]]

    "$BIN" touch mode.txt
    "$BIN" chmod 600 mode.txt
    [[ "$(stat -c '%a' mode.txt)" == '600' ]]
    "$BIN" sync
    id_output="$("$BIN" id)"
    [[ "$id_output" == uid=* ]]
    [[ "$id_output" == *' gid='* ]]

    printf 'a b\nc\n' > wc.txt
    [[ "$("$BIN" wc wc.txt)" == '2 3 6' ]]

    for n in {1..12}; do
        printf 'line%s\n' "$n"
    done > all-lines.txt
    for n in {1..10}; do
        printf 'line%s\n' "$n"
    done > expected-head.txt
    "$BIN" head all-lines.txt > actual-head.txt
    cmp expected-head.txt actual-head.txt

    [[ "$("$BIN" echo symlink-dispatch)" == 'symlink-dispatch' ]]
    [[ "$("$BIN_LINKS/echo" symlink-dispatch)" == 'symlink-dispatch' ]]

    "$BIN" rm sample.txt input.txt moved.txt input-link mode.txt wc.txt
)

printf 'Flatcore smoke tests passed.\n'
