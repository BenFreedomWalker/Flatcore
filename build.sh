#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${1:-"$ROOT_DIR/build"}"
RUN_TESTS=0

if [[ "${2:-}" == "--test" ]]; then
    RUN_TESTS=1
elif [[ $# -gt 1 ]]; then
    printf 'Usage: %s [build-directory] [--test]\n' "$0" >&2
    exit 2
fi

case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$ROOT_DIR/$BUILD_DIR" ;;
esac

BIN_DIR="$BUILD_DIR/bin"
FASM_BIN="${FASM:-fasm}"

if ! command -v "$FASM_BIN" >/dev/null 2>&1; then
    printf 'error: FASM was not found. Install flat assembler or set FASM=/path/to/fasm.\n' >&2
    exit 1
fi

mkdir -p "$BUILD_DIR" "$BIN_DIR"
(
    cd "$ROOT_DIR"
    "$FASM_BIN" src/flatcore.asm "$BUILD_DIR/flatcore"
)

commands=(
    help echo true false pwd cat ls
    mkdir rmdir rm touch cp mv clear uname
    basename dirname head wc ln readlink
)

for command_name in "${commands[@]}"; do
    ln -sfn ../flatcore "$BIN_DIR/$command_name"
done

printf 'Built: %s\n' "$BUILD_DIR/flatcore"
printf 'Installed %d command links in: %s\n' "${#commands[@]}" "$BIN_DIR"
printf 'Try: %s/flatcore help\n' "$BUILD_DIR"

if (( RUN_TESTS )); then
    "$ROOT_DIR/tests/smoke.sh" "$BUILD_DIR"
fi
