#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build}"
case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$ROOT_DIR/$BUILD_DIR" ;;
esac
BIN="$BUILD_DIR/flatcore"
BIN_LINKS="$BUILD_DIR/bin"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

[[ -x "$BIN" ]] || { printf 'error: missing executable: %s\n' "$BIN" >&2; exit 1; }
"$BIN" true
if "$BIN" false; then
    printf 'false returned success unexpectedly\n' >&2
    exit 1
fi

expected='Flatcore smoke test'
actual="$("$BIN" echo "$expected")"
[[ "$actual" == "$expected" ]]
[[ "$("$BIN" echo -n "$expected")" == "$expected" ]]
[[ "$("$BIN" basename '/tmp/flatcore-test/')" == 'flatcore-test' ]]
[[ "$("$BIN" dirname '/tmp/flatcore-test/file.txt')" == '/tmp/flatcore-test' ]]
[[ "$("$BIN" pwd)" == "$(pwd -P)" ]]
[[ "$("$BIN" basename 'C:\folder\file.txt')" == 'file.txt' ]]
[[ "$("$BIN" basename '/')" == '/' ]]
[[ "$("$BIN" basename 'C:\')" == '\' ]]
[[ "$("$BIN" dirname 'file.txt')" == '.' ]]
[[ "$("$BIN" dirname '/file.txt')" == '/' ]]
[[ "$("$BIN" dirname 'C:\folder\file.txt')" == 'C:\folder' ]]
[[ "$("$BIN" dirname 'C:\file.txt')" == 'C:\' ]]
if "$BIN" pwd unexpected >/dev/null 2>&1; then
    printf 'pwd accepted an unexpected operand\n' >&2
    exit 1
fi

(
    cd "$TMP_DIR"
    [[ "$("$BIN" pwd)" == "$TMP_DIR" ]]
    "$BIN" touch sample.txt
    "$BIN" echo 'assembly userspace' > input.txt
    printf 'first\000chunk\n' > cat-first.bin
    printf 'second\r\n' > cat-second.bin
    : > cat-empty.bin
    cat cat-first.bin cat-second.bin > cat-expected.bin
    "$BIN" cat cat-first.bin cat-second.bin > cat-actual.bin
    cmp cat-expected.bin cat-actual.bin
    "$BIN" cat cat-empty.bin > cat-empty-output.bin
    cmp cat-empty.bin cat-empty-output.bin
    "$BIN" cat < cat-first.bin > cat-stdin-output.bin
    cmp cat-first.bin cat-stdin-output.bin
    "$BIN" cat - < cat-second.bin > cat-dash-output.bin
    cmp cat-second.bin cat-dash-output.bin
    if "$BIN" cat missing-cat-file cat-first.bin > cat-after-error.bin 2>/dev/null; then
        printf 'cat succeeded despite a missing input file\n' >&2
        exit 1
    fi
    cmp cat-first.bin cat-after-error.bin
    "$BIN" cp input.txt copied.txt
    cmp input.txt copied.txt
    "$BIN" mv copied.txt moved.txt
    cmp input.txt moved.txt

    "$BIN" mkdir ls-dir
    printf 'visible\n' > ls-dir/visible
    printf 'secret\n' > ls-dir/.hidden
    printf 'first\n' > ls-file-one
    printf 'second\n' > ls-file-two
    printf 'leading\n' > ./-ls-leading
    "$BIN" ls ls-dir > ls-default.txt
    grep -qx 'visible' ls-default.txt
    if grep -q '\.hidden' ls-default.txt; then
        printf 'ls showed a hidden file by default\n' >&2
        exit 1
    fi
    "$BIN" ls -la ls-dir > ls-long.txt
    grep -q '^d---------' ls-long.txt
    grep -q '\.hidden
    "$BIN" ls --help > ls-help.txt
    grep -q -- '--almost-all' ls-help.txt
    if "$BIN" ls --not-a-real-option >/dev/null 2>&1; then
        printf 'ls accepted an unknown option\n' >&2
        exit 1
    fi

    "$BIN" rm ls-dir/visible ls-dir/.hidden ls-file-one ls-file-two ./-ls-leading
    "$BIN" rmdir ls-dir
    "$BIN" mkdir sample-dir
    "$BIN" rmdir sample-dir

    "$BIN" ln -s input.txt input-link
    [[ "$("$BIN" readlink input-link)" == 'input.txt' ]]
    "$BIN" ln input.txt hard-link
    cmp input.txt hard-link

    "$BIN" touch mode.txt
    "$BIN" chmod 600 mode.txt
    [[ "$(stat -c '%a' mode.txt)" == '600' ]]
    "$BIN" sync
    id_output="$("$BIN" id)"
    [[ "$id_output" == uid=* ]]
    [[ "$id_output" == *' gid='* ]]
    [[ -n "$("$BIN" hostname)" ]]
    [[ "$("$BIN" uptime)" == *' seconds' ]]
    "$BIN" sleep 0

    printf 'a b\nc\n' > wc.txt
    [[ "$("$BIN" wc wc.txt)" == '2 3 6' ]]

    for n in {1..12}; do printf 'line%s\n' "$n"; done > all-lines.txt
    for n in {1..10}; do printf 'line%s\n' "$n"; done > expected-head.txt
    "$BIN" head all-lines.txt > actual-head.txt
    cmp expected-head.txt actual-head.txt

    [[ "$("$BIN" echo symlink-dispatch)" == 'symlink-dispatch' ]]
    [[ "$("$BIN_LINKS/echo" symlink-dispatch)" == 'symlink-dispatch' ]]

    "$BIN" rm sample.txt input.txt moved.txt input-link hard-link mode.txt wc.txt
    if "$BIN" rm missing-file; then
        printf 'rm succeeded for a missing file\n' >&2
        exit 1
    fi
    if "$BIN" cat missing-file >/dev/null 2>&1; then
        printf 'cat succeeded for a missing file\n' >&2
        exit 1
    fi
)

sleep 30 &
child_pid=$!
"$BIN" kill "$child_pid"
wait "$child_pid" 2>/dev/null || true
printf 'Flatcore Linux smoke tests passed.\n'
 ls-long.txt
    grep -q 'visible
    "$BIN" ls --help > ls-help.txt
    grep -q -- '--almost-all' ls-help.txt
    if "$BIN" ls --not-a-real-option >/dev/null 2>&1; then
        printf 'ls accepted an unknown option\n' >&2
        exit 1
    fi

    "$BIN" rm ls-dir/visible ls-dir/.hidden
    "$BIN" rmdir ls-dir
    "$BIN" mkdir sample-dir
    "$BIN" rmdir sample-dir

    "$BIN" ln -s input.txt input-link
    [[ "$("$BIN" readlink input-link)" == 'input.txt' ]]
    "$BIN" ln input.txt hard-link
    cmp input.txt hard-link

    "$BIN" touch mode.txt
    "$BIN" chmod 600 mode.txt
    [[ "$(stat -c '%a' mode.txt)" == '600' ]]
    "$BIN" sync
    id_output="$("$BIN" id)"
    [[ "$id_output" == uid=* ]]
    [[ "$id_output" == *' gid='* ]]
    [[ -n "$("$BIN" hostname)" ]]
    [[ "$("$BIN" uptime)" == *' seconds' ]]
    "$BIN" sleep 0

    printf 'a b\nc\n' > wc.txt
    [[ "$("$BIN" wc wc.txt)" == '2 3 6' ]]

    for n in {1..12}; do printf 'line%s\n' "$n"; done > all-lines.txt
    for n in {1..10}; do printf 'line%s\n' "$n"; done > expected-head.txt
    "$BIN" head all-lines.txt > actual-head.txt
    cmp expected-head.txt actual-head.txt

    [[ "$("$BIN" echo symlink-dispatch)" == 'symlink-dispatch' ]]
    [[ "$("$BIN_LINKS/echo" symlink-dispatch)" == 'symlink-dispatch' ]]

    "$BIN" rm sample.txt input.txt moved.txt input-link hard-link mode.txt wc.txt
    if "$BIN" rm missing-file; then
        printf 'rm succeeded for a missing file\n' >&2
        exit 1
    fi
    if "$BIN" cat missing-file >/dev/null 2>&1; then
        printf 'cat succeeded for a missing file\n' >&2
        exit 1
    fi
)

sleep 30 &
child_pid=$!
"$BIN" kill "$child_pid"
wait "$child_pid" 2>/dev/null || true
printf 'Flatcore Linux smoke tests passed.\n'
 ls-long.txt
    "$BIN" ls -A ls-dir > ls-almost.txt
    grep -q '^\.hidden
    "$BIN" ls --help > ls-help.txt
    grep -q -- '--almost-all' ls-help.txt
    if "$BIN" ls --not-a-real-option >/dev/null 2>&1; then
        printf 'ls accepted an unknown option\n' >&2
        exit 1
    fi

    "$BIN" rm ls-dir/visible ls-dir/.hidden
    "$BIN" rmdir ls-dir
    "$BIN" mkdir sample-dir
    "$BIN" rmdir sample-dir

    "$BIN" ln -s input.txt input-link
    [[ "$("$BIN" readlink input-link)" == 'input.txt' ]]
    "$BIN" ln input.txt hard-link
    cmp input.txt hard-link

    "$BIN" touch mode.txt
    "$BIN" chmod 600 mode.txt
    [[ "$(stat -c '%a' mode.txt)" == '600' ]]
    "$BIN" sync
    id_output="$("$BIN" id)"
    [[ "$id_output" == uid=* ]]
    [[ "$id_output" == *' gid='* ]]
    [[ -n "$("$BIN" hostname)" ]]
    [[ "$("$BIN" uptime)" == *' seconds' ]]
    "$BIN" sleep 0

    printf 'a b\nc\n' > wc.txt
    [[ "$("$BIN" wc wc.txt)" == '2 3 6' ]]

    for n in {1..12}; do printf 'line%s\n' "$n"; done > all-lines.txt
    for n in {1..10}; do printf 'line%s\n' "$n"; done > expected-head.txt
    "$BIN" head all-lines.txt > actual-head.txt
    cmp expected-head.txt actual-head.txt

    [[ "$("$BIN" echo symlink-dispatch)" == 'symlink-dispatch' ]]
    [[ "$("$BIN_LINKS/echo" symlink-dispatch)" == 'symlink-dispatch' ]]

    "$BIN" rm sample.txt input.txt moved.txt input-link hard-link mode.txt wc.txt
    if "$BIN" rm missing-file; then
        printf 'rm succeeded for a missing file\n' >&2
        exit 1
    fi
    if "$BIN" cat missing-file >/dev/null 2>&1; then
        printf 'cat succeeded for a missing file\n' >&2
        exit 1
    fi
)

sleep 30 &
child_pid=$!
"$BIN" kill "$child_pid"
wait "$child_pid" 2>/dev/null || true
printf 'Flatcore Linux smoke tests passed.\n'
 ls-almost.txt
    if grep -qx '\.' ls-almost.txt || grep -qx '\.\.' ls-almost.txt; then
        printf 'ls -A included dot entries\n' >&2
        exit 1
    fi
    "$BIN" ls -a ls-dir > ls-all.txt
    grep -qx '\.' ls-all.txt
    grep -qx '\.\.' ls-all.txt
    [[ "$("$BIN" ls -d ls-dir)" == 'ls-dir' ]]
    "$BIN" ls ls-file-one ls-file-two > ls-multiple.txt
    grep -qx 'ls-file-one' ls-multiple.txt
    grep -qx 'ls-file-two' ls-multiple.txt
    "$BIN" ls -- ./-ls-leading > ls-leading.txt
    grep -qx './-ls-leading' ls-leading.txt
    "$BIN" ls --version > ls-version.txt
    grep -q 'Flatcore ls' ls-version.txt
    "$BIN" ls --help > ls-help.txt
    grep -q -- '--almost-all' ls-help.txt
    if "$BIN" ls --not-a-real-option >/dev/null 2>&1; then
        printf 'ls accepted an unknown option\n' >&2
        exit 1
    fi

    "$BIN" rm ls-dir/visible ls-dir/.hidden
    "$BIN" rmdir ls-dir
    "$BIN" mkdir sample-dir
    "$BIN" rmdir sample-dir

    "$BIN" ln -s input.txt input-link
    [[ "$("$BIN" readlink input-link)" == 'input.txt' ]]
    "$BIN" ln input.txt hard-link
    cmp input.txt hard-link

    "$BIN" touch mode.txt
    "$BIN" chmod 600 mode.txt
    [[ "$(stat -c '%a' mode.txt)" == '600' ]]
    "$BIN" sync
    id_output="$("$BIN" id)"
    [[ "$id_output" == uid=* ]]
    [[ "$id_output" == *' gid='* ]]
    [[ -n "$("$BIN" hostname)" ]]
    [[ "$("$BIN" uptime)" == *' seconds' ]]
    "$BIN" sleep 0

    printf 'a b\nc\n' > wc.txt
    [[ "$("$BIN" wc wc.txt)" == '2 3 6' ]]

    for n in {1..12}; do printf 'line%s\n' "$n"; done > all-lines.txt
    for n in {1..10}; do printf 'line%s\n' "$n"; done > expected-head.txt
    "$BIN" head all-lines.txt > actual-head.txt
    cmp expected-head.txt actual-head.txt

    [[ "$("$BIN" echo symlink-dispatch)" == 'symlink-dispatch' ]]
    [[ "$("$BIN_LINKS/echo" symlink-dispatch)" == 'symlink-dispatch' ]]

    "$BIN" rm sample.txt input.txt moved.txt input-link hard-link mode.txt wc.txt
    if "$BIN" rm missing-file; then
        printf 'rm succeeded for a missing file\n' >&2
        exit 1
    fi
    if "$BIN" cat missing-file >/dev/null 2>&1; then
        printf 'cat succeeded for a missing file\n' >&2
        exit 1
    fi
)

sleep 30 &
child_pid=$!
"$BIN" kill "$child_pid"
wait "$child_pid" 2>/dev/null || true
printf 'Flatcore Linux smoke tests passed.\n'
