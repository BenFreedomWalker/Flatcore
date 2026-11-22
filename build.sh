#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ACTION=build
BUILD_DIR="$ROOT_DIR/build"

case "${1:-}" in
    build|test|install|clean)
        ACTION=$1
        BUILD_DIR="${2:-$BUILD_DIR}"
        if [[ $# -gt 2 ]]; then
            printf 'Usage: %s [build|test|install|clean] [build-directory]\n' "$0" >&2
            exit 2
        fi
        ;;
    "")
        ;;
    *)
        BUILD_DIR=$1
        if [[ "${2:-}" == "--test" ]]; then
            ACTION=test
        elif [[ "${2:-}" == "--install" ]]; then
            ACTION=install
        elif [[ $# -gt 1 ]]; then
            printf 'Usage: %s [build-directory] [--test|--install]\n' "$0" >&2
            exit 2
        fi
        ;;
esac

case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$ROOT_DIR/$BUILD_DIR" ;;
esac

if [[ "$ACTION" == clean ]]; then
    rm -rf -- "$BUILD_DIR"
    printf 'Removed build directory: %s\n' "$BUILD_DIR"
    exit 0
fi

FASM_BIN="${FASM:-fasm}"
if ! command -v "$FASM_BIN" >/dev/null 2>&1; then
    printf 'error: FASM was not found. Install flat assembler or set FASM=/path/to/fasm.\n' >&2
    exit 127
fi

BIN_DIR="$BUILD_DIR/bin"
mkdir -p "$BIN_DIR"
(
    cd "$ROOT_DIR"
    "$FASM_BIN" src/flatcore.asm "$BUILD_DIR/flatcore"
)

commands=(
    help echo true false pwd cat ls
    mkdir rmdir rm touch cp mv clear uname
    basename dirname head wc ln readlink
    chmod kill sync id
)

for command_name in "${commands[@]}"; do
    ln -sfn ../flatcore "$BIN_DIR/$command_name"
done

printf 'Built: %s\n' "$BUILD_DIR/flatcore"
printf 'Created %d command links in: %s\n' "${#commands[@]}" "$BIN_DIR"
printf 'Try: %s/flatcore help\n' "$BUILD_DIR"

if [[ "$ACTION" == test ]]; then
    "$ROOT_DIR/tests/smoke.sh" "$BUILD_DIR"
fi

if [[ "$ACTION" == install ]]; then
    PREFIX="${PREFIX:-/usr/local}"
    DESTDIR="${DESTDIR:-}"
    INSTALL_BIN="${DESTDIR}${PREFIX}/bin"
    mkdir -p "$INSTALL_BIN"
    cp "$BUILD_DIR/flatcore" "$INSTALL_BIN/flatcore"
    chmod 755 "$INSTALL_BIN/flatcore"
    for command_name in "${commands[@]}"; do
        ln -sfn flatcore "$INSTALL_BIN/$command_name"
    done
    printf 'Installed Flatcore into: %s\n' "$INSTALL_BIN"
fi
